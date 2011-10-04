#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/CommandLine.h"
#include "SKIR/SKIRRuntime.h"
#include "SKIRRuntimeGraph.h"
#include "SKIRMergeSched.h"

//#include "inline_stream_ops.h"

#include <fstream>

int MaxMergeSize;
static llvm::cl::opt<int, true>
FakeMaxMergeSize("max-merge",
		 llvm::cl::desc(""),
		 llvm::cl::location(MaxMergeSize), llvm::cl::init(10000));

namespace llvm {

SKIRMergeSched::merge_set_t *
SKIRMergeSched::kernel_map_find(SKIRRuntimeKernel *rtk)
{
    merge_sets_t::iterator I, E;
    for (I = merge_sets.begin(), E = merge_sets.end(); I!=E; ++I) {
	merge_set_t *m = *I;
	SKIRRuntimeKernel *k = m->first;
	if (k == rtk)
	    return m;
	kernel_set_t &kernel_set = m->second;
	if (kernel_set.count(rtk))
	    return m;
    }
    return NULL;
}

// append m to the list of merge sets
void 
SKIRMergeSched::kernel_map_append(merge_set_t *m)
{
    merge_sets.push_back(m);
}

// insert rtk into merge set m
void
SKIRMergeSched::kernel_map_insert(SKIRRuntimeKernel *rtk, merge_set_t *m)
{
    kernel_set_t &kernel_set = m->second;
    kernel_set.insert(rtk);
}

// replace the rtk in m with k
void
SKIRMergeSched::kernel_map_replace(SKIRRuntimeKernel *k, merge_set_t *m)
{
    m->first = k;
}

void
SKIRMergeSched::kernel_map_erase(merge_set_t *ms)
{
    merge_sets_t::iterator I, E;
    for (I = merge_sets.begin(), E = merge_sets.end(); I!=E; ++I) {
	merge_set_t *m = *I;
	if (ms == m) {
	    ms->first = 0;
	    kernel_set_t &ks = ms->second;
	    ks.clear();
	    merge_sets.erase(I);
	    return;
	}
    }
}

SKIRRuntimeKernel*
SKIRMergeSched::tryMerge(SKIRRuntimeKernel *rtk)
{
    int nin = rtk->nins;
#if 1
    bool is_dp = /*!rtk->is_stateful &&*/ !rtk->has_peek;
    if (!is_dp)
	return NULL;
#endif
    if (!rtk->is_const_idx)
	return NULL;

    // for each input stream s
    SKIRRuntimeKernel *new_rtk = 0;
    for (int i = 0; i<nin; i++) {
	// if other end of s is known
	skir_stream_t *s = (skir_stream_t*)rtk->impl_ins[i];
	SKIRRuntimeKernel *src_rtk = (SKIRRuntimeKernel *)s->src;
	merge_set_t *m = kernel_map_find(src_rtk);
	if (m) {
	    // try to merge with other end of s
	    SKIRRuntimeKernel *k = m->first;
	    kernel_set_t &kset = m->second;
	    if (kset.size() >= MaxMergeSize) continue;
	    if (k == rtk) continue;
	    if (!k->is_const_idx) continue;
	    if (/*k->is_stateful ||*/ k->has_peek) continue;
	    if (k->nouts > 2) continue;
	    if (rtk->nins > 2) continue;

	    new_rtk = sg->fuseKernels(k, rtk);

	    // if merge worked, add rtk to the merge set
	    if (new_rtk) {
		// if rtk is already in a merge set, remove it
		if (merge_set_t *ms = kernel_map_find(rtk)) {
		    kernel_map_erase(ms);
		    delete ms;
		}
		// add rtk to the merge set
		kernel_map_insert(rtk, m);
		//kernel_map_insert(new_rtk, m);

		// change merge set representitive kernel
		kernel_map_replace(new_rtk, m);

		return new_rtk;
	    }
	}
    }

    return new_rtk;
}

void
SKIRMergeSched::callKernel(SKIRRuntimeKernel *rtk)
{
    // try to merge rtk with its inputs if they exist and have been called
    SKIRRuntimeKernel *k = tryMerge(rtk);

    // if rtk cannot merge with any of its neighbors, start a new kernel set
    if (!k) {
	if (!kernel_map_find(rtk)) {
	    //errs() << "new merge set: "
	    //<<  rtk->work->getName() << "\n";
	    merge_set_t *m = new merge_set_t;
	    m->first = rtk;
	    kernel_map_insert(rtk, m);
	    kernel_map_append(m);
	}
    }
    else {
	k->sched = this;
	callKernel(k);
    }
}

void
SKIRMergeSched::waitKernel(SKIRRuntimeKernel *rtk)
{

    merge_set_t *m = kernel_map_find(rtk);
    SKIRRuntimeKernel *k = m->first;
    assert(k);
    sched->waitKernel(k);
}

// stop executing rtk, and remove internal state associated with it
void
SKIRMergeSched::removeKernel(SKIRRuntimeKernel *rtk)
{
}

void
SKIRMergeSched::pauseKernel(SKIRRuntimeKernel *rtk)
{
    merge_set_t *m = kernel_map_find(rtk);
    SKIRRuntimeKernel *k = m->first;
    assert(k);
    sched->pauseKernel(k);
}

void
SKIRMergeSched::unPauseKernel(SKIRRuntimeKernel *rtk)
{
    merge_set_t *m = kernel_map_find(rtk);
    SKIRRuntimeKernel *k = m->first;
    assert(k);
    sched->unPauseKernel(k);
}

void
SKIRMergeSched::runCodeGen(SKIRRuntimeKernel *rtk)
{
    merge_set_t *m = kernel_map_find(rtk);
    SKIRRuntimeKernel *k = m->first;
    assert(k);
    sched->runCodeGen(k);
}

struct skir_merge_thread {
    SKIRMergeSched &s;
    skir_merge_thread(SKIRMergeSched &sched) : s(sched) {}
    void operator()() { s.run(); }
};

void
SKIRMergeSched::start(void)
{
    if (running.compare_and_swap(1,0) == 0) {
    }
}

void
SKIRMergeSched::stop(void)
{
    if (running.compare_and_swap(0,1) == 1) {
    }
}

void
SKIRMergeSched::run()
{
    if (running.compare_and_swap(1,0) == 0) {
	sched = sg->getTbbSched();
	//sg->log();
	// stop merging and start executing
	merge_sets_t::iterator I, E;
	for (I = merge_sets.begin(), E = merge_sets.end(); I!=E; ++I) {
	    merge_set_t *m = *I;
	    SKIRRuntimeKernel *k = m->first;
	    if (k) {
		k->sched = sched;
		//k->work->dump();
		sched->start();
		sched->callKernel(k);
	    }
	}
    }
}


}//namespace llvm
