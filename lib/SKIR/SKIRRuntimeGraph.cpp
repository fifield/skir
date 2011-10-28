
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include "llvm/Support/CommandLine.h"
#include <llvm/Intrinsics.h>
#include <llvm/IntrinsicInst.h>

#include <vector>
#include <assert.h>
#include <fstream>

#include <SKIR/SKIRRuntime.h>
#include "SKIRRuntimeGraph.h"
#include "SKIRUtil.h"
#include "SKIRCommandLine.h"
#include "SKIRFusion.h"
#include "SKIRFission.h"

#include "SKIRTbbSched.h"
#include "SKIRDPSched.h"
#include "SKIRMergeSched.h"
#include "SKIRSingleThreadSched.h"
#include "SKIRKoroSched.h"

#ifdef USE_OPENCL
#include "SKIROpenCLSched.h"
#endif

using namespace llvm;
using namespace std;

//
// command line options are here
//
bool EnableOpenCLSched;
static llvm::cl::opt<bool, true>
FakeEnableOpenCL("enable-opencl",
		 llvm::cl::desc("enable opencl scheduler"),
		 llvm::cl::location(EnableOpenCLSched), llvm::cl::init(false));

int OpenCLMultiplier;
static llvm::cl::opt<int, true>
FakeOpenCLMultiplier("opencl-mult",
		     llvm::cl::desc(""),
		     llvm::cl::location(OpenCLMultiplier), llvm::cl::init(1));

bool ForceSingleSched;
static llvm::cl::opt<bool, true>
FakeForceSingle("force-single",
		 llvm::cl::desc("only use the single threaded scheduler"),
		 llvm::cl::location(ForceSingleSched), llvm::cl::init(false));

bool ForceTPK;
static llvm::cl::opt<bool, true>
FakeForceTPK("force-tpk",
	     llvm::cl::desc("use a single thread for each kernel"),
	     llvm::cl::location(ForceTPK), llvm::cl::init(false));

bool EnableDPSched;
static llvm::cl::opt<bool, true>
FakeEnableDP("enable-dp",
	     llvm::cl::desc("enable data parallel scheduler"),
	     llvm::cl::location(EnableDPSched), llvm::cl::init(false));

int DPWidth;
static llvm::cl::opt<int, true>
FakeDPWidth("dp-width",
	     llvm::cl::desc("width of data parallel scheduler"),
	     llvm::cl::location(DPWidth), llvm::cl::init(1));

bool llvm::EnableMergeSched;
static llvm::cl::opt<bool, true>
FakeEnableMerge("enable-merge",
		llvm::cl::desc("enable kernel fusion"),
		llvm::cl::location(EnableMergeSched), llvm::cl::init(false));

int MergeThresh;
static llvm::cl::opt<int, true>
FakeMergeThresh("merge-thresh",
		llvm::cl::desc("merge threshold in cycles"),
		llvm::cl::location(MergeThresh), llvm::cl::init(100));

// float ParallelThresh;
// static llvm::cl::opt<float, true>
// FakeParallelThresh("parallel-thresh",
// 		   llvm::cl::desc("minimum amount of parallelism to maintain (default = 1.0)"),
// 		   llvm::cl::location(ParallelThresh), llvm::cl::init(1.0));


//------------------------------------------------
//  RuntimeGraph
//------------------------------------------------

SKIRRuntimeGraph::SKIRRuntimeGraph(SKIRRuntime &runtime)
    : rt(runtime), verbose(false)
{
    adj.reserve(10240);

    the_tbb_sched = new SKIRTbbSched(this, rt.getNumThreads());

    the_koro_sched = new SKIRKoroSched(this);

    if (EnableOpenCLSched) {
#ifdef USE_OPENCL	
	the_opencl_sched = new SKIROpenCLSched(this);
	the_opencl_sched->setVerbose(false);
#else
	assert(0 && "OpenCL requested, but OpenCL was not enabled at compile time");
#endif
    }

    if (EnableDPSched) {
	the_dp_sched= new SKIRDPSched(this, rt.getNumThreads());
    }
    
    // if (EnableMergeSched) {
    // 	the_merge_sched = new SKIRMergeSched(this);
    // }

    logfile_id = 0;
    hier_parent = 0;
}

SKIRRuntimeGraph::~SKIRRuntimeGraph()
{
#ifdef USE_OPENCL
    if (the_opencl_sched) {
	the_opencl_sched->stop();
	delete the_opencl_sched;
	the_opencl_sched = 0;
    }
#endif
    if (the_merge_sched) {
	the_merge_sched->stop();
	delete the_merge_sched;
	the_merge_sched = 0;
    }
    if (the_dp_sched) {
	the_dp_sched->stop();
	delete the_dp_sched;
	the_dp_sched = 0;
    }
}

void
SKIRRuntimeGraph::setVerbose(bool v)
{ 
    verbose = v;
    if (the_tbb_sched) the_tbb_sched->setVerbose(v);
#ifdef USE_OPENCL
    if (the_opencl_sched) the_opencl_sched->setVerbose(v);
#endif
    if (the_dp_sched) the_dp_sched->setVerbose(v);
    
    if (the_koro_sched) the_koro_sched->setVerbose(v);
    
}

// SKIRRuntimeGraph::callKernel - handle a skir.call instruction
//  - add the kernel to the stream graph
//  - add new edges to the stream graph
//  - set the scheduler and codegen to use to use
//  - allocate streams
//  - pass the kernel to a scheduler for execution
void
SKIRRuntimeGraph::callKernel(SKIRRuntimeKernel *kernel)
{
    double t_begin,t_end;
    rdtod(t_begin);

    //
    // setup the kernel if it hasn't been called before
    //
    if (kernel->base_work == kernel->work) {  // XXX: this if statement sucks
	
	// - set the backend to use
	kernel->cg = rt.getCG();
	
	// make runtime copies of work function and adjust prototypes
	SKIRRuntime::runSKIRCloneWorkPass(kernel);

	// run kernel analysis if required
	if (!kernel->opt_only)
	    SKIRRuntime::runSKIRKernelInfoPass(kernel);

	// - add the kernel and new edges to the stream graph
	addKernel(kernel);

	if (kernel->is_hier) {
	    codeGenKernel(kernel);
	} 
	else {
	    // - set the scheduler to use
	    selectScheduler(kernel);
	}
	
	if (hier_parent) {
	    hier_parent->children.push_back(kernel);
	}
    }

    // 
    // do the call
    // 
    if (verbose) errs() << "SKIRRuntimeGraph::callKernel: "
			<<  kernel->work->getName() << "\n";

    if (kernel->is_hier) {
	rdtod(t_end);

	// run the work function now
	SKIRRuntimeKernel *pop = hier_parent;
	hier_parent = kernel;
	kernel->workfn(&kernel->rt_state, kernel->state, kernel->rt_ins, kernel->rt_outs);
	hier_parent = pop;
    }
    else {
	// - allocate streams
	allocateStreams(kernel);
	rdtod(t_end);

	// - pass the instruction to a scheduler for execution
	kernel->sched->callKernel(kernel);
    }

    kernel->total_jit_time += (t_end - t_begin);
}

// add kernel to the wait list, put the thread to sleep
void
SKIRRuntimeGraph::waitKernel(SKIRRuntimeKernel *kernel)
{
    if (verbose) errs() << "SKIRRuntimeGraph::waitKernel: "
			<< kernel->base_work->getName() << "\n";

    static bool first = true;
    bool was_first = first;
    first = false;
    if (was_first) { 
	//log();
    }
    
    //if (EnableMergeSched)
    //((SKIRMergeSched*)the_merge_sched)->run();
    
    if (kernel->is_hier) {
	std::vector<SKIRRuntimeKernel*>::iterator I,E;
	I = kernel->children.begin();
	E = kernel->children.end();
	for ( ; I!=E; ++I) {
	    SKIRRuntimeKernel *k = *I;
	    waitKernel(k);
	}
    }
    else {
	kernel->sched->waitKernel(kernel);
    }

#if 1
    if (was_first) {
	ofstream o;
	o.open("run.dot");
	dot(o);
	o.close();
    }
#endif

#if 0
    if (was_first) {
	ofstream o;
	o.open("run.ll");
	Module *mod = kernel->work->getParent();
	raw_os_ostream oo(o);
	mod->print(oo, 0);
	o.close();
	//exit(1);
    }
#endif
}

void
SKIRRuntimeGraph::becomeKernel(SKIRRuntimeKernel *kernel, std::vector<SKIRRuntimeKernel*> &kernels)
{
    
}

void
SKIRRuntimeGraph::pauseKernel(SKIRRuntimeKernel *kernel)
{
    assert(kernel && kernel->sched);
    kernel->sched->pauseKernel(kernel);
}

void
SKIRRuntimeGraph::unPauseKernel(SKIRRuntimeKernel *kernel)
{
    assert(kernel && kernel->sched);
    kernel->sched->unPauseKernel(kernel);
}

//
//
void
SKIRRuntimeGraph::selectScheduler(SKIRRuntimeKernel *kernel, SKIRScheduler *fix_sched)
{
    if (fix_sched) {
	kernel->sched = fix_sched;
	kernel->fixed_sched = true;
	return;
    } 
    else if (kernel->fixed_sched) {
	assert(kernel->sched);
	return;
    }

    kernel->sched = 0;

    if (ForceSingleSched) {
	kernel->sched = the_koro_sched;
	if (verbose) errs() << "KO: " << kernel->base_work->getName() << "\n";
	kernel->sched->setVerbose(verbose);
	kernel->sched->start();
	return;
    }

    if (ForceTPK) {
	kernel->opt_only = true;
	kernel->sched = new SKIRSingleThreadSched(this);
	if (verbose) errs() << "ST: " << kernel->base_work->getName() << "\n";
	kernel->sched->setVerbose(verbose);
	kernel->sched->start();
	return;
    }

    // if (EnableMergeSched) {
    // 	kernel->sched = the_merge_sched;
    // 	if (verbose) errs() << "MRG: " << kernel->base_work->getName() << "\n";
    // 	return;
    //}
    
    if (kernel->is_fixed_rate) {
	bool is_dp = !kernel->is_stateful && !kernel->has_peek;
	if (EnableDPSched && is_dp) {
	    kernel->sched = the_dp_sched;
	    if (verbose) errs() << "DP: " << kernel->base_work->getName() << "\n";
	    return;
	}
#ifdef USE_OPENCL
	else if (EnableOpenCLSched && is_dp) {
	    the_opencl_sched->start();
	    kernel->sched = the_opencl_sched;
	    if (verbose) errs() << "CL: " << kernel->base_work->getName() << "\n";
	    return;
	}
	else goto deflt;
#endif
    }
 deflt:	
    kernel->sched = rt.getSched(); // tbb sched
    kernel->sched->start();
    if (verbose) errs() << "TBB: " << kernel->base_work->getName() << "\n";
     
    assert(kernel->sched && "couldn't find an appropriate scheduler for kernel");
}

// generate executable code for a kernel work function.
// the passes to run before codegen the codegen to use are stored in the kernel.
//
void
SKIRRuntimeGraph::codeGenKernel(SKIRRuntimeKernel *kernel)
{
    if (kernel->fpm) {
	MutexGuard locked(kernel->cg->lock);
	kernel->fpm->run(*kernel->work);
	delete kernel->fpm;
	kernel->fpm = 0;
    } else {
	 kernel->fpm = new FunctionPassManager(kernel->work->getParent());
	 kernel->fpm->add(new TargetData(kernel->work->getParent()));
	 SKIRRuntime::addLLVMOpts(kernel);
	 codeGenKernel(kernel);
	 return;
    }
    
    void *fp = kernel->cg->getPointerToFunction(kernel->work);//, host_arch);//, size);
    kernel->workfn = reinterpret_cast<work_function*>(reinterpret_cast<uintptr_t>(fp));
}

// find limiter kernel
// 
SKIRRuntimeKernel *
SKIRRuntimeGraph::getLimiter()
{
    unsigned long long total = 0;
    SKIRRuntimeKernel *max = 0;
    std::map< SKIRRuntimeKernel*,unsigned > t;

    std::map< unsigned, SKIRRuntimeKernel* >::iterator I,E;
    for (I = id2kernel.begin(), E = id2kernel.end(); I!=E; ++I) {
	SKIRRuntimeKernel *k = (*I).second;
	if (!k || k->total_niter < 16)
	    return 0;
	t[k] = k->total_runtime / k->total_niter;
	total += t[k];
	if (!max || (t[k] > t[max])) max = k;
    }
    if (t[max] > (total / rt.getNumThreads()))
	max = 0;
    return max;
}

// perform kernel fission
//
void
SKIRRuntimeGraph::fissKernel(std::vector<SKIRRuntimeKernel*> &outK, 
			     SKIRRuntimeKernel *kernel, int width)
{
    SKIRFission fission(*this);

    {
	MutexGuard locked(kernel->cg->lock);
	fission.runOnKernel(outK, kernel, width);
    }

    width = outK.size();
    if (width) {
	removeKernel(kernel);
	for (int i=0; i<width; i++)
	    addKernel(outK[i]);
    }
}

// fuse two kernels in the graph and do the equiv of
// callKernel on the resulting merged kernel
//
SKIRRuntimeKernel *
SKIRRuntimeGraph::fuseKernels(SKIRRuntimeKernel *kernel0,
			      SKIRRuntimeKernel *kernel1)
{
    SKIRFusion fuse(*this);
    //SKIRScheduler *sched = kernel0->sched;
    //ExecutionEngine *cg = kernel0->cg;

    assert(kernel0->sched == kernel1->sched);
    assert(kernel0->cg == kernel1->cg);

    SKIRRuntimeKernel *k = 0;
    {
	MutexGuard locked(kernel0->cg->lock);
	k = fuse.runOnKernels(*kernel0, *kernel1);
    }
    if (k) {
	removeKernel(kernel0);
	removeKernel(kernel1);
	addKernel(k);
    }

    return k;
}

// allocate implementations details for the streams
//
void
SKIRRuntimeGraph::allocateStreams(SKIRRuntimeKernel *kernel)
{
    // only allow one call per kernel instance
    assert(!kernel->impl_ins && !kernel->impl_outs && "streams already allocated!\n");

    int nin = kernel->nins;
    int nout = kernel->nouts;

    // allocate inputs
    skir_stream_t** kins = new skir_stream_t*[nin+1];
    for (int i=0; i<nin; i++) {
	SKIRRuntimeStream **sptr = kernel->rt_ins;
	SKIRRuntimeStream *s = sptr[i];
	s->is_src = 1;
	// already allocated when other end of stream seen first
	if (!s->si) s->si = new_skir_stream_t(s);
	kins[i] = s->si;
	if (s->type != SKIRRuntimeStream::SHARED)
	    s->si->dst = kernel;
	else
	    s->si->dst = (void*)2;
	s->si->pop_rate = s->getPopRate();
	s->si->peek_rate = s->getPeekRate();
	for (int j=0; j<NUM_STREAM_HEADERS; j++) {
	    s->si[-(1+j)].pop_rate = s->getPopRate();
	    s->si[-(1+j)].peek_rate = s->getPeekRate();
	}

    }
    kins[nin] = 0;
    kernel->impl_ins = (void **)kins;

    // allocate outputs
    skir_stream_t** kouts = new skir_stream_t*[nout+1];
    for (int i=0; i<nout; i++) {
	SKIRRuntimeStream **sptr = kernel->rt_outs;
	SKIRRuntimeStream *s = sptr[i];
	s->is_dst = 1;
	// already allocated when other end of stream seen first
	if (!s->si) s->si = new_skir_stream_t(s);
	kouts[i] = s->si;
	if (s->type != SKIRRuntimeStream::SHARED)
	    s->si->src = kernel;
	else
	    s->si->src = (void*)2;
	s->si->push_rate = s->getPushRate();
	for (int j=0; j<NUM_STREAM_HEADERS; j++) {
	    s->si[-(1+j)].push_rate = s->getPushRate();
	}
    }
    kouts[nout] = 0;
    kernel->impl_outs = (void **)kouts;
}


//
// graphish operations
//

// add a kernel to the graph
void
SKIRRuntimeGraph::addKernel(SKIRRuntimeKernel *kernel)
{
    assert(kernel->id < adj.capacity());

    if (id2kernel[kernel->id] == kernel)
	return;

    if (verbose) errs() << "addKernel: " << kernel->work->getName() << "\n";

    adj[kernel->id].clear();
    id2kernel[kernel->id] = kernel;

    //refreshAdjList();
}

// remove kernel from the graph
void
SKIRRuntimeGraph::removeKernel(SKIRRuntimeKernel *kernel)
{
    adj[kernel->id].clear();
    id2kernel.erase(kernel->id);
}

// add a directed edge to the graph
void
SKIRRuntimeGraph::addEdge(SKIRRuntimeKernel *src, SKIRRuntimeKernel *dst)
{
    if ((src == (SKIRRuntimeKernel*)-1) || (src == (SKIRRuntimeKernel*)1) || !src) return;
    if ((dst == (SKIRRuntimeKernel*)-1) || (dst == (SKIRRuntimeKernel*)1) || !dst) return;
    if (!id2kernel[src->id]) return;

    if (verbose) errs() << "addEdge: " << src->id << ", " << dst->id << "\n";

    adj[src->id].push_back(dst->id);
}

void
SKIRRuntimeGraph::log(void)
{
    string logfile_prefix="/tmp/skir_";
    stringstream logfile_name;
    ofstream logfile;

    logfile_name << logfile_prefix << getpid() << "_" << logfile_id++ << ".dot";
    logfile.open(logfile_name.str().c_str());

    refreshAdjList();
    dot(logfile);

    logfile.close();
}

void
SKIRRuntimeGraph::refreshAdjList(void)
{
    std::set< SKIRRuntimeStream* > visited;

    for (unsigned int i=0; i<adj.capacity(); i++)
	adj[i].clear();

    std::map< unsigned, SKIRRuntimeKernel* >::iterator I,E;
    for (I = id2kernel.begin(), E = id2kernel.end(); I!=E; ++I) {
	SKIRRuntimeKernel *kernel = (*I).second;
	if (!kernel) continue;

	// add the streams/edges to the stream graph
	for (int i=0; i<kernel->nins; i++) {
	    SKIRRuntimeStream *s = kernel->rt_ins[i];
	    if (visited.count(s)) continue;
	    visited.insert(s);
	    if (!s->si) continue;
	    SKIRRuntimeKernel *src = (SKIRRuntimeKernel*)s->si->src;
	    SKIRRuntimeKernel *dst =  (SKIRRuntimeKernel*)s->si->dst;
	    addEdge(src,dst);
	}
	for (int i=0; i<kernel->nouts; i++) {
	    SKIRRuntimeStream *s = kernel->rt_outs[i];
	    if (visited.count(s)) continue;
	    visited.insert(s);
	    if (!s->si) continue;
	    SKIRRuntimeKernel *src = (SKIRRuntimeKernel*)s->si->src;
	    SKIRRuntimeKernel *dst =  (SKIRRuntimeKernel*)s->si->dst;
	    addEdge(src,dst);
	}
    }
}

void
SKIRRuntimeGraph::dot(std::ostream &o)
{
    unsigned long long total_runtime = 0;
    unsigned long long total_bytes = 0;
    unsigned long long total_jit_time = 0;
    double min = 100000000;
    double max = 0;

    //refreshAdjList();

    o << "digraph {\n"; //<< "rankdir=\"LR\"\n";
    std::map< unsigned, SKIRRuntimeKernel* >::iterator I,E;
    for (I = id2kernel.begin(), E = id2kernel.end(); I!=E; ++I) {
	int id = (*I).first;
	SKIRRuntimeKernel *k = (*I).second;
	if (!k) continue;

	for (unsigned int j=0; j<adj[id].size(); j++) {
	    if (!id2kernel[ adj[id][j] ]) continue;
	    o << k->id << " -> " << id2kernel[ adj[id][j] ]->id << "\n";
	    //o << /*k->work->getNameStr() << ":" <<*/ k->id << " -> "
	    //	      << id2kernel[ adj[i][j] ]->work->getNameStr() /*<< ":"*/
	    //<< id2kernel[ adj[i][j] ]->id << "\n";
	}
    }

    for (I = id2kernel.begin(), E = id2kernel.end(); I!=E; ++I) {
	SKIRRuntimeKernel *k = (*I).second;
	if (!k) continue;

	total_runtime += k->total_runtime;
	total_bytes += k->total_bytes;
	total_jit_time += k->total_jit_time;

	double d = (double)k->total_bytes / (double)k->total_runtime;
	if (d<min) min = d;
	if (d>max) max = d;
    }

    for (I = id2kernel.begin(), E = id2kernel.end(); I!=E; ++I) {
	SKIRRuntimeKernel *k = (*I).second;
	if (!k) continue;

	o << /*k->work->getNameStr() << ":" <<*/ k->id \
	  << " [label=<" << k->work->getNameStr() \
	  << " <BR/>\n  total_runtime = " << k->total_runtime \
	  << " <BR/>\n  total_jit_time = " << k->total_jit_time \
	  << " <BR/>\n  total_niter = "	<< k->total_niter \
	  << " <BR/>\n  total_ncall = "	<< k->total_ncall \
	  << " <BR/>\n  total_bytes = "	<< k->total_bytes \
	  << " <BR/>\n  cycles_per_iter = " \
	  << k->total_runtime/(k->total_niter?k->total_niter:-1) \
	  << " <BR/>\n  cycles_per_call = " \
	  << k->total_runtime/(k->total_ncall?k->total_ncall:-1) \
	  << " <BR/>\n  affinity = " << k->affinity;
	for (int i=0; i<48; i++) {
	    if (!k->total_runtime_per_thread[i]) continue;
	    o << " <BR/>\n  cycles_per_thread_ " << i << " = "	\
	      << k->total_runtime_per_thread[i];
	}
	o << ">];\n";
	
    }
    o << "}" << "\n";
    o << "# total cycles: " << total_runtime << "\n";
    o << "# total jit time: " << total_jit_time << "\n";
    o << "# nkern: " << id2kernel.size() << "\n";
    o << "# min: " << min << "\n";
    o << "# max: " << max << "\n";
    o << "# aggr: " << (double)(total_bytes) / (double)(total_runtime) << "\n";
}

void
SKIRRuntimeGraph::topo_sort(list<SKIRRuntimeKernel *>& sorted)
{
    refreshAdjList();

    int n = adj.capacity();
    bool *c = new bool[n];

    for (int i=0; i<n; i++)
	c[i] = false;

    for (int i=0; i<n; i++)
	if (!c[i]) topo_sort_visit(i, c, sorted);

    delete[] c;
}

void
SKIRRuntimeGraph::topo_sort_visit(int k, bool *c, list<SKIRRuntimeKernel *>& sorted)
{
    c[k] = true;

    if (id2kernel.count(k) == 0)
	return;

    for (int i=adj[k].size()-1; i>=0; i--)
	if (!c[adj[k][i]])
	    topo_sort_visit(adj[k][i], c, sorted);

    sorted.push_front(id2kernel[k]);
}
