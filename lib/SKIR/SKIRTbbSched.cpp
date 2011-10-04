
#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/CommandLine.h"

#include <SKIR/SKIRRuntime.h>
#include <SKIR/SKIRStream.h>

#include "SKIRKoroSched.h"
#include "SKIRTbbSched.h"
#include "SKIRRuntimeGraph.h"
#include "SKIRTiming.h"
#include "SKIRCommandLine.h"

#include "tbb/tbb.h"
#include <tbb/spin_mutex.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/task_scheduler_observer.h>
#include <tbb/enumerable_thread_specific.h>

#include <pthread.h>

#include <sys/time.h>
#include <map>
#include <sys/syscall.h>

#define _GNU_SOURCE
#include <sched.h>

#include "SKIR_kernel_t.h"

namespace llvm {

static cl::opt<int>
TbbRetries("tbb-retries",
	   cl::desc("tbb retries"), cl::init(-1));

// < thread affinity, tbb affinity >
typedef tbb::enumerable_thread_specific< std::pair<int,int> > AffinityPair;
static AffinityPair affinity_pair(std::make_pair(-1,-1));

class MyObserver: public tbb::task_scheduler_observer {

    tbb::atomic<int> my_cpuid;

public:
    MyObserver( )  {
        observe(true);
	my_cpuid = 0;
    }

    void on_scheduler_entry( bool is_worker ) {
	int id = 0;
	if (is_worker) id = (my_cpuid.fetch_and_increment() + 1);

	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(id, &set);
	sched_setaffinity(0, sizeof(set), &set);

	//errs() << "entry " << id << "\n";
	AffinityPair::reference pair = affinity_pair.local();
	pair.first = id;
    }
    
    void on_scheduler_exit( bool is_worker ) {
	//errs() << "exit" << is_worker << "\n";
    }
};

static MyObserver observer;

//
// kernel_map & helpers
//   kernel_map maps SKIRRuntimeKernels (i.e. skir inst operands)
//   to a corresponding kernel_t (tbbsched information)
//

// hash_map<int> kernel_map 
static kernel_map_t kernel_map;

// kernel_map[key]
static kernel_t*
kernel_map_find(SKIRRuntimeKernel *key)
{
    kernel_map_t::const_accessor a;
    if (kernel_map.find(a,key))
	return a->second;
    return NULL;
}

// kernel_map[key] = value
static void
kernel_map_insert(SKIRRuntimeKernel *key, kernel_t *value)
{
    kernel_map_t::accessor a;
    kernel_map.insert(a, key);
    a->second = value;
}

//
// kernel_task
//

class kernel_task : public tbb::task
{
    kernel_t *k;

public:

    kernel_task(kernel_t *kernel) : k(kernel) {
	kernel_lock_t::scoped_lock l;
	assert(!l.try_acquire(k->lock) && "must hold kernel->lock in kernel_task constructor");
	assert(k->running > 0);
	k->stats.num_tasks++;
	k->owning_task = this;
	k->fail = 0;
	k->retries = TbbRetries;
    }

    static task *new_kernel_task(kernel_task *t, kernel_t *k) {
	task *new_t = new(t->task::allocate_additional_child_of(*t->parent())) kernel_task(k);
	//assert(k->rt_kernel.affinity >= 0);
	//new_t->set_affinity(k->rt_kernel.affinity);
	return new_t;
    }

    void recycle(void) {
	k->stats.num_continue++;
	set_ref_count(1);
	recycle_as_safe_continuation();
    }

    void note_affinity(affinity_id id) {
	AffinityPair::reference pair = affinity_pair.local();
	if (pair.second == -1) {
	    pair.second = id;
	    //SKIRTbbSched *s = (SKIRTbbSched *)k->rt_kernel.sched;
	    //s->setAffinities();
	} else assert(pair.second == id);
    }

    // 
    // try to perform dynamic kernel fusion
    //
    static SKIRRuntimeKernel* merge_cb(void *v, SKIRRuntimeKernel *me, SKIRRuntimeKernel *r)
    {
	SKIRRuntimeGraph *sg = (SKIRRuntimeGraph *)v;

	//SKIRRuntimeKernel *limiter = sg->getLimiter();
	//if(limiter) errs() << "LIMITER: " << limiter->work->getName() << "\n";
	    
	if (r == 0) return r;
	if ((size_t)r & 1) return r;

	// only kernels that have run for a while
	if (r->total_niter < 32) return r;
	if (me->total_niter < 32) return r;
	
	// only for kernels with input streams
	if (me->nins && (sg->getRuntime().getNumThreads()*2 < sg->getNumKernels())) {
	    int blk_idx = -1;
	    for (int j=0; j<me->nins; j++) {
		// for now, the two kernels can share only one common stream.
		// this is to avoid fusing across a stream with data in it.
		if (me->rt_ins[j]->si->src == r) {
		    if (blk_idx == -1)
			blk_idx = j;
		    else
			return r;
		}
	    }

	    // only kernels that are blocked on an empty input stream
	    if (blk_idx >= 0) {

		// only kernels with very small runtimes
		if (me->total_runtime / me->total_niter > 256) return r;
		if (r->total_runtime / r->total_niter > 256) return r;

		SKIRRuntimeKernel *new_rtk = 0;
		kernel_t *blocker = kernel_map_find(r);
		kernel_lock_t::scoped_lock l;
		if (l.try_acquire(blocker->lock)) {
		    if (me->rt_ins[blk_idx]->si->head == me->rt_ins[blk_idx]->si->tail) {
			Function *old_me_work = me->work;
			Function *old_r_work = r->work;
			me->work = me->base_work;
			r->work = r->base_work;
			SKIRRuntime::runSKIRCloneWorkPass(r);
			SKIRRuntime::runSKIRCloneWorkPass(me);
			new_rtk = sg->fuseKernels(r, me);
			if (new_rtk) {
			    me->sched->removeKernel(me);
			    r->sched->removeKernel(r);

			    SKIRRuntime::runLLVMOpts(new_rtk);
			    SKIRRuntime::runSKIRCloneWorkPass(new_rtk);

			    std::vector<SKIRRuntimeKernel*>::iterator I,E;
			    for (I=me->children.begin(), E=me->children.end(); I!=E; ++I) {
				SKIRRuntimeKernel *child = *I;
				new_rtk->children.push_back(child);
			    }
			    for (I=r->children.begin(), E=r->children.end(); I!=E; ++I) {
				SKIRRuntimeKernel *child = *I;
				new_rtk->children.push_back(child);
			    }

			    new_rtk->sched->callKernel(new_rtk);
			    return new_rtk;
			}
			// failed to merge
			me->work = old_me_work;
			r->work = old_r_work;
		    }
		}
	    }
	}
#if 0
	// only for kernels with a single output stream
	if (me->nouts == 1) {

	    // kernel is blocked on a full output stream
	    if (me->rt_outs[0]->si->dst == r) {

		SKIRRuntimeKernel *limiter = sg->getLimiter();
		if (limiter == r) {
		    errs() << "LIMITER: " << limiter->work->getName() << "\n";
		}
		return r;
	    
		// only state free, fixed rate kernels
		//if (r->is_stateful || !r->is_fixed_rate) return r;

		// only kernels with large runtimes
		if (r->total_niter && r->total_runtime / r->total_niter < 10240) return r;

		// only single in, single out kernels
		if (r->nins != 1) return r;
		if (r->nouts != 1) return r;

		// try to get a lock on the blocking kernel
		kernel_t *blocker = kernel_map_find(r);
		kernel_lock_t::scoped_lock l;
		if (l.try_acquire(blocker->lock)) {
		    //skir_stream_t *s = me->rt_outs[0]->si;
		    //size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
		    //if (next == s->tail) then it is full
		    
		    me->work = me->base_work;
		    std::vector<SKIRRuntimeKernel *> outK;
		    sg->fissKernel(outK, r, 4);
		    errs() << "yah\n";
		    if (outK.size()) {
			me->sched->removeKernel(me);
			for (int i=0; i<outK.size(); i++) {
			    me->sched->callKernel(outK[i]);
			}
			return outK[0];
		    }
		}
	    }
	}
#endif
	return r;
    }

    task *execute()
    {
	kernel_lock_t::scoped_lock lock;
	if (lock.try_acquire(k->lock)) {

	    if ( !((k->owning_task == this) && k->is_active()) ) {
		if (k->owning_task != this)
		    k->stats.num_steal++;
		assert(k->running > 0);
		k->running--;
		return NULL;
	    }

	    if (!k->rt_kernel.workfn) {
		k->rt_kernel.sched->runCodeGen(&k->rt_kernel);
		assert(k->rt_kernel.workfn);
	    }

	    int retries = TbbRetries;

	    while (1) {
		if (!kernel_map_find(&k->rt_kernel)) break;

		k->rt_kernel.rt_state->cycles = 0;

		// k->work returns 0 to be rescheduled, 1 if it is finished.
		// otherwise, it returns a pointer to a blocking kernel.
		SKIRRuntimeKernel *b = k->work();
		while (b == 0) {
		    b = k->work();
		}

		AffinityPair::reference pair = affinity_pair.local();
		if (pair.second != -1) 
		    k->rt_kernel.total_runtime_per_thread[pair.second] += 
			k->rt_kernel.rt_state->cycles;

		// if blocked on finished kernel
		if (b == (SKIRRuntimeKernel *)1) {
		    k->done();
		}
		else if (kernel_t *blocker = kernel_map_find(b)) {
		    kernel_lock_t::scoped_lock l;
		    if (l.try_acquire(blocker->lock)) {
			if (blocker->running == 0) {
			    blocker->running++;
			    recycle();
			    tbb::task *t = new_kernel_task(this, blocker);
			    return t;
			}
			else {
			    if (!DisableKoroSteal)
				if (tbb::task *t = find_and_steal_task(*blocker->owning_task)) {
				    recycle();
				    return t;
				}
			}
		    }
		    // didn't get the lock, assume now blocked on running kernel
		    if (retries-- > 0) {
			k->stats.num_retries++;
			continue;
		    }
		}
		break;
	    } // end while(1)

	    // if we get here, task is done
	    assert(k->running > 0);
	    k->running--;

	} // unlock
	else {
	    // failed to get lock
	    assert(k->running > 0);
	    k->running--;
	}
	return NULL;
    }
};

///
/// SKIRTbbSched  
///

SKIRTbbSched::SKIRTbbSched(SKIRRuntimeGraph *stream_graph, int nthreads) : sg(stream_graph),
									   num_workers(nthreads),
									   verbose(false),
									   root_task(NULL),
									   main_thread(NULL)
{
    running = 0;
    root_task = new(tbb::task::allocate_root()) tbb::empty_task;
    root_task->increment_ref_count();
    if (TbbRetries == -1) {
	if (num_workers > 1) {
	    TbbRetries = 0;
	} else {
	    TbbRetries = 1;
	}
    }
}

void
SKIRTbbSched::runCodeGen(SKIRRuntimeKernel *rtk)
{
    SKIRKoroSched::genericCodeGen(sg, rtk);
}

void
SKIRTbbSched::waitKernel(SKIRRuntimeKernel *rt_kernel)
{
    kernel_t *k = kernel_map_find(rt_kernel);
    assert(k);
    k->wait(DONE);
}

void
SKIRTbbSched::setAffinities()
{
    std::list<SKIRRuntimeKernel *> l;
    sg->topo_sort(l);
    
    int c = l.size() / sg->getRuntime().getNumThreads();
    int i = 0, j = 0;
    std::list<SKIRRuntimeKernel *>::iterator I = l.begin(), E = l.end();
    if (verbose) errs() << "------ set affinities --------\n";
    for (; I!=E; ++I) {
	if (j++ > c) {
	    j = 0;
	    i++;
	}
	AffinityPair::iterator AI = affinity_pair.begin(), AE = affinity_pair.end();
	int aff = 0;
	for (; AI!=AE; ++AI)
	    if ((*AI).first == i)
		aff = ((*AI).second != -1) ? (*AI).second : 0;
	(*I)->affinity = aff;
	if (verbose) errs() << (*I)->work->getName() << "  " << i << "  " << aff << "\n";
    }
    if (verbose) errs() << "----- end affinities ---------\n";
}

void
SKIRTbbSched::callKernel(SKIRRuntimeKernel *rt_kernel)
{
    //    errs() << "SKIRTbbSched callKernel: " << rt_kernel->work->getName() << "\n";
    kernel_t *k = new kernel_t(*rt_kernel);
    assert(k);
    kernel_map_insert(rt_kernel, k);
    k->active();
    runq.push(k);
    if (EnableMergeSched) {
	k->niter_cb = kernel_task::merge_cb;
	k->niter_cb_data = sg;
    }
    //setAffinities();
}

void
SKIRTbbSched::removeKernel(SKIRRuntimeKernel *rt_kernel)
{
    kernel_t *k = kernel_map_find(rt_kernel);
    if (k) {
	k->owning_task = 0;
	kernel_map_insert(rt_kernel, 0);
	// XXX should delete k here but it's not ref counted
	// so we can't.  instead it leaks
    }
}

void
SKIRTbbSched::pauseKernel(SKIRRuntimeKernel *rt_kernel)
{
    kernel_t *k = kernel_map_find(rt_kernel);
    assert(k);
    k->pause();
}

void
SKIRTbbSched::unPauseKernel(SKIRRuntimeKernel *rt_kernel)
{
    kernel_t *k = kernel_map_find(rt_kernel);
    assert(k);
    k->unpause();
}

struct skir_tbb_thread {
    SKIRTbbSched &s;
    skir_tbb_thread(SKIRTbbSched &sched) : s(sched) {}
    void operator()() { s.run(); }
};

void
SKIRTbbSched::start(void)
{
    if (running.compare_and_swap(1,0) == 0) {
	main_thread = new tbb::tbb_thread(*(new skir_tbb_thread(*this)));
	assert(main_thread);
    }
}

void
SKIRTbbSched::stop(void)
{
    if (!main_thread) return;

    if (running.compare_and_swap(0,1) == 1) {
	assert(main_thread && main_thread->joinable() && "not running");
	main_thread->join();
	main_thread = NULL;
    }
}

void 
SKIRTbbSched::run()
{
    tbb::task_scheduler_init tbb_init(num_workers);

    while (running == 1) {
	kernel_t *k = 0;
	if (runq.try_pop(k)) {
	    if (kernel_map_find(&k->rt_kernel) && k->is_active()) {
		tbb::task *t = 0;
		if (k->running == 0) {
		    kernel_lock_t::scoped_lock lock;
		    if (lock.try_acquire(k->lock)) {

			if (!k->rt_kernel.workfn) {
			    k->rt_kernel.sched->runCodeGen(&k->rt_kernel);
			    assert(k->rt_kernel.workfn);
			}

			if (k->running == 0) {
			    k->running++;
			    t = new (root_task->allocate_child()) kernel_task(k);
			    assert(k->rt_kernel.affinity >= 0);
			    //t->set_affinity(k->rt_kernel.affinity);
			}
		    }
		}
		if (t) {
		    root_task->increment_ref_count();
		    root_task->spawn(*t);
		    if (root_task->ref_count() >= runq.size()) {
		    	root_task->wait_for_all();
		    	root_task->increment_ref_count();
		    }
		}
	    }
	    if (!k->is_done()) {
		if (kernel_map_find(&(k->rt_kernel)) == k)
		    runq.push(k);
	    }
	} else {
	    // runq is empty, yield
	    tbb::this_tbb_thread::yield();
	}
    }
}

} //namespace llvm
