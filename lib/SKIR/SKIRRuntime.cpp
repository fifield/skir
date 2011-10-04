
#include <llvm/Module.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Intrinsics.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/System/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/StandardPasses.h>
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constant.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Support/SourceMgr.h"

using namespace llvm;
#include "SKIRRuntimeGraph.h"
#include "SKIRDPSched.h"
#include "SKIRTbbSched.h"
#include "SKIRMergeSched.h"
#include "SKIRUtil.h"
#include <SKIR/SKIRRuntime.h>

#include <fstream>

// skir/build/lib/events.pb.h
#include "events.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace llvm;
using namespace std;

extern "C" {
#include <stdio.h>

/* these are defined at the end of this file */
void *__SKIRRT_kernel(void *me, void *init, void *work, void *args);
void __SKIRRT_call(void *me, void *kernel, void *in_streams, void *out_stream);
void __SKIRRT_wait(void *me, void *kernel);
void __SKIRRT_become(void *me, void *k, void *in_streams, void *out_stream);
void *__SKIRRT_stream(void *me, unsigned int elem_size);
void *__SKIRRT_array(void *me, void *begin, void *end, unsigned int elem_size, unsigned int stride);
void __SKIR_push(skir_stream_idx_t p, skir_stream_element_t e);
void __SKIR_pop(skir_stream_idx_t p, skir_stream_element_t e);

/* these are in SKIRKoroSched */
void __SKIRRT_yield64(void *from_rtk, void *to_rtk);
void __SKIRRT_return64(void *rt_state, int ret_val);
}

#include "inline_stream_ops.cpp"

//------------------------------------------------
//  SKIRRuntime Class
//------------------------------------------------

SKIRRuntime::SKIRRuntime()
{
    initialized = false;

    host_mod = 0;
    host_cg = 0;
    host_sg = 0;
    host_sched = 0;
    num_threads = 1;
    
    next_kernel_id = 0;
    next_stream_id = 1;

    verbose = false;

}

SKIRRuntime::~SKIRRuntime()
{
    if (host_sched) delete host_sched;
    host_sched = 0;

    if (host_sg) delete host_sg;
    host_sg = 0;
}

void
SKIRRuntime::setVerbose(bool v)
{
    if (getSG()) getSG()->setVerbose(v);
    if (getSched()) getSched()->setVerbose(v);
    verbose = v;
}

// 
// Implementation of the skir.kernel instruction.
// The arguments are a pointer to the init function, a pointer to the work
// function and a pointer to the argument to the init function.
// The main job of the kernel instruction is to construct the runtime's 
// internal representation of the kernel and call the init function.
//
void *
SKIRRuntime::handleKernelInst(Function *init, Function *work, void *args)
{
    SKIRRuntimeKernel *k = new SKIRRuntimeKernel(nextKernelID());

    // set the work function
    k->base_work = k->work = work;

    // set the init function
    k->base_init = k->init = init;

    // run the kernel's init function
    runInitFunction(k, args);

    return (void *)k;
}

void *
SKIRRuntime::handleKernelInst(void *init, void *work, void *args)
{
    Function *w = fn_map[work];
    Function *i = fn_map[init];
    assert(i&&w);

    return handleKernelInst(i, w, args);
}


void
SKIRRuntime::addLLVMOpts(SKIRRuntimeKernel *k)
{
    FunctionPassManager &PM = *k->fpm;
    
    createStandardFunctionPasses(&PM, 3);

    // Start of function pass.
    
    PM.add(createScalarReplAggregatesPass());  // Break up aggregate allocas
    //if (SimplifyLibCalls)
    //PM.add(createSimplifyLibCallsPass());    // Library Call Optimizations
    PM.add(createInstructionCombiningPass());  // Cleanup for scalarrepl.
    PM.add(createJumpThreadingPass());         // Thread jumps.
    PM.add(createCFGSimplificationPass());     // Merge & remove BBs
    PM.add(createInstructionCombiningPass());  // Combine silly seq's
    
    PM.add(createTailCallEliminationPass());   // Eliminate tail calls
    PM.add(createCFGSimplificationPass());     // Merge & remove BBs
    PM.add(createReassociatePass());           // Reassociate expressions
    PM.add(createLoopRotatePass());            // Rotate Loop
    PM.add(createLICMPass());                  // Hoist loop invariants
    //PM.add(createLoopUnswitchPass(OptimizeSize || OptimizationLevel < 3));
    PM.add(createInstructionCombiningPass());  
    PM.add(createIndVarSimplifyPass());        // Canonicalize indvars
    PM.add(createLoopDeletionPass());          // Delete dead loops
    //if (UnrollLoops)
    PM.add(createLoopUnrollPass());          // Unroll small loops
    PM.add(createInstructionCombiningPass());  // Clean up after the unroller
    //if (OptimizationLevel > 1)
    PM.add(createGVNPass());                 // Remove redundancies
    PM.add(createMemCpyOptPass());             // Remove memcpy / form memset
    PM.add(createSCCPPass());                  // Constant prop with SCCP

    // Run instcombine after redundancy elimination to exploit opportunities
    // opened up by them.
    PM.add(createInstructionCombiningPass());
    PM.add(createJumpThreadingPass());         // Thread jumps
    PM.add(createDeadStoreEliminationPass());  // Delete dead stores
    PM.add(createAggressiveDCEPass());         // Delete dead instructions
    PM.add(createCFGSimplificationPass());     // Merge & remove BBs
    PM.add(createSCCVNPass());

    PM.add(createVerifierPass());
}

void
SKIRRuntime::runLLVMOpts(SKIRRuntimeKernel *k)
{

    Function *F = k->work;
    FunctionPassManager PM(F->getParent());
    PM.add(new TargetData(F->getParent()));
    
    createStandardFunctionPasses(&PM, 3);

    // Start of function pass.
    
    PM.add(createScalarReplAggregatesPass());  // Break up aggregate allocas
    //if (SimplifyLibCalls)
    //PM.add(createSimplifyLibCallsPass());    // Library Call Optimizations
    PM.add(createInstructionCombiningPass());  // Cleanup for scalarrepl.
    PM.add(createJumpThreadingPass());         // Thread jumps.
    PM.add(createCFGSimplificationPass());     // Merge & remove BBs
    PM.add(createInstructionCombiningPass());  // Combine silly seq's
    
    PM.add(createTailCallEliminationPass());   // Eliminate tail calls
    PM.add(createCFGSimplificationPass());     // Merge & remove BBs
    PM.add(createReassociatePass());           // Reassociate expressions
    PM.add(createLoopRotatePass());            // Rotate Loop
    PM.add(createLICMPass());                  // Hoist loop invariants
    //PM.add(createLoopUnswitchPass(OptimizeSize || OptimizationLevel < 3));
    PM.add(createInstructionCombiningPass());  
    PM.add(createIndVarSimplifyPass());        // Canonicalize indvars
    PM.add(createLoopDeletionPass());          // Delete dead loops
    //if (UnrollLoops)
    PM.add(createLoopUnrollPass());          // Unroll small loops
    PM.add(createInstructionCombiningPass());  // Clean up after the unroller
    //if (OptimizationLevel > 1)
    PM.add(createGVNPass());                 // Remove redundancies
    PM.add(createMemCpyOptPass());             // Remove memcpy / form memset
    PM.add(createSCCPPass());                  // Constant prop with SCCP

    // Run instcombine after redundancy elimination to exploit opportunities
    // opened up by them.
    PM.add(createInstructionCombiningPass());
    PM.add(createJumpThreadingPass());         // Thread jumps
    PM.add(createDeadStoreEliminationPass());  // Delete dead stores
    PM.add(createAggressiveDCEPass());         // Delete dead instructions
    PM.add(createCFGSimplificationPass());     // Merge & remove BBs
    PM.add(createSCCVNPass());

    PM.add(createVerifierPass());

    {
	MutexGuard locked(k->cg->lock);
	PM.run(*F);
    }
}

// 
// SKIRRuntime::handleCallInst - entry point for skir.call instructions
//  - execute hierarchical kernels on host
//  - run final arch independ SKIR passes (none currently)
//  - pass the instruction to a stream graph for execution
void
SKIRRuntime::handleCallInst(void *k, void *is, void *os)
{
    SKIRRuntimeKernel *kernel = (SKIRRuntimeKernel *)k;
    SKIRRuntimeStream **ins = (SKIRRuntimeStream **)is;
    SKIRRuntimeStream **outs = (SKIRRuntimeStream **)os;

    kernel->nins = 0;
    for (SKIRRuntimeStream **sptr = ins; sptr && *sptr; sptr++)
	kernel->nins++;

    kernel->nouts = 0;
    for (SKIRRuntimeStream **sptr = outs; sptr && *sptr; sptr++)
	kernel->nouts++;

    kernel->rt_ins = new SKIRRuntimeStream*[kernel->nins+1];
    kernel->rt_outs = new SKIRRuntimeStream*[kernel->nouts+1];
    
    for (int i=0; i<kernel->nins; i++)
	kernel->rt_ins[i] = ins[i];
    kernel->rt_ins[kernel->nins] = 0;

    for (int i=0; i<kernel->nouts; i++)
	kernel->rt_outs[i] = outs[i];
    kernel->rt_outs[kernel->nouts] = 0;

    if (verbose) {
	errs() << "handleCall: ";
	errs() << kernel->work->getNameStr() << " " << kernel << "\n";
    }

    // add to stream graph for execution
    getSG()->callKernel(kernel);
}

void
SKIRRuntime::handleBecomeInst(void *k, void *is, void *os)
{
    SKIRRuntimeKernel *new_rtk = (SKIRRuntimeKernel *)k;
    SKIRRuntimeKernel *old_rtk = 0;
    skir_stream_t **impl_ins = (skir_stream_t **)is;
    skir_stream_t **impl_outs = (skir_stream_t **)os;

    std::vector<SKIRRuntimeStream *> ins;
    std::vector<SKIRRuntimeStream *> outs;

    // locate and disconnect old kernel
    // build vectors of RuntimeStream: ins, outs
    for (skir_stream_t **sptr = impl_ins; sptr && *sptr; sptr++) {
	skir_stream_t *si = *sptr;
	SKIRRuntimeStream *rs = (SKIRRuntimeStream *)si->rs;
	ins.push_back(rs);
	if (old_rtk) assert(old_rtk == (SKIRRuntimeKernel*)si->dst);
	else old_rtk = (SKIRRuntimeKernel*)si->dst;
	si->dst = (void*)-1;
    }
    
    for (skir_stream_t **sptr = impl_outs; sptr && *sptr; sptr++) {
	skir_stream_t *si = *sptr;
	SKIRRuntimeStream *rs = (SKIRRuntimeStream *)si->rs;
	outs.push_back(rs);
	if (old_rtk) assert(old_rtk == (SKIRRuntimeKernel*)si->src);
	else old_rtk = (SKIRRuntimeKernel*)si->src;
	si->src = (void*)-1;
    }

    old_rtk->sched->removeKernel(old_rtk);
    delete[] old_rtk->impl_ins;
    delete[] old_rtk->impl_outs;
    old_rtk->impl_ins = 0;
    old_rtk->impl_outs = 0;

    // setup new kernel
    new_rtk->nins = old_rtk->nins;
    new_rtk->nouts = old_rtk->nouts;
    new_rtk->rt_ins = new SKIRRuntimeStream*[new_rtk->nins+1];
    new_rtk->rt_outs = new SKIRRuntimeStream*[new_rtk->nouts+1];
    
    for (int i=0; i<new_rtk->nins; i++)
	new_rtk->rt_ins[i] = ins[i];
    new_rtk->rt_ins[new_rtk->nins] = 0;

    for (int i=0; i<new_rtk->nouts; i++)
	new_rtk->rt_outs[i] = outs[i];
    new_rtk->rt_outs[new_rtk->nouts] = 0;

    if (verbose) {
	errs() << "handleBecome: ";
	errs() << old_rtk->work->getNameStr() << " -> " << new_rtk->work->getNameStr() << "\n";
    }

    getSG()->callKernel(new_rtk);
}

void
SKIRRuntime::runInitFunction(SKIRRuntimeKernel *k, void *args)
{
    void *init = getCG()->getPointerToFunction(k->init);
    
    // call k->init() on host
    init_function* initfn = reinterpret_cast<init_function*>(reinterpret_cast<uintptr_t>(init));
    k->state = initfn(args);
}

//
// Allocate a new SKIRRuntimeStream
//
SKIRRuntimeStream *
SKIRRuntime::newStream()
{
    SKIRRuntimeStream *stream = new SKIRRuntimeStream(next_stream_id++);
    return stream;
}

//
// implement skir.stream instruction
//
void *
SKIRRuntime::handleStreamInst(unsigned elem_size)
{
    SKIRRuntimeStream *rs = newStream();
    rs->type = SKIRRuntimeStream::NATIVE;
    rs->elem_size = elem_size;
    return (void *)rs;
}

//
// implement skir.array instruction
//
void *
SKIRRuntime::handleArrayInst(void *begin, void *end, unsigned elem_size, unsigned stride)
{
    SKIRRuntimeStream *rs = newStream();
    assert(0 && "array inst");
    //rs->type = ARRAY;
    rs->begin = begin;
    rs->end = end;
    rs->stride = stride;
    rs->elem_size = elem_size;
    return (void *)rs;

}

void
SKIRRuntime::includeModule(Module *mod)
{
    MutexGuard locked(getCG()->lock);
    assert(mod && mod->MaterializeAll() == false);

    runSKIRLoweringPasses(this, mod);

    CloneModuleInto(mod, getModule());
}

void
SKIRRuntime::handleWaitInst(void *k)
{
    SKIRRuntimeKernel *kernel = (SKIRRuntimeKernel *)k;

    if (verbose) {
	errs() << "handleWait: " << kernel->is_hier << " ";
	errs() << kernel->work->getNameStr() << " " << kernel << "\n";
    }

    getSG()->waitKernel(kernel);

    if (verbose) {
	errs() << "handleWait returned: " << kernel->is_hier << " ";
	errs() << kernel->work->getNameStr() << " " << kernel << "\n";
    }
}

void
SKIRRuntime::addSymbol(const char *name, vfp symbol)
{
    MutexGuard locked(getCG()->lock);
    sys::DynamicLibrary::AddSymbol(name, (void*)symbol);
}

//
//
//
void
SKIRRuntime::initialize(int nthreads)
{
    if (initialized)
	return;

    LLVMContext &CTX = getGlobalContext();
    string errormsg;
    
    setNumThreads(nthreads);

    atexit(llvm_shutdown);  // Call llvm_shutdown() on exit.
    InitializeNativeTarget();

    // load the runtime bitcode library
    string skir_root(getenv("SKIR_ROOT"));
    string filename = skir_root+"/skir/build/lib/inline_stream_ops.bc";
    if (MemoryBuffer *buffer = MemoryBuffer::getFileOrSTDIN(filename, &errormsg)) {
	setModule(ParseBitcodeFile(buffer, CTX, &errormsg));
	delete buffer;
    }
    if (!getModule()) {
	errs() << "bitcode: '" << filename << "' didn't read correctly:";
	if (errormsg.size()) errs() << errormsg;
	errs() << "\n";
	assert(0);
    }

    // If we are supposed to override the target triple for the host, do so now.
    //if (!TargetTriple.empty())
    //Mod->setTargetTriple(TargetTriple);

    // create the default JIT ExecutionEngine for the host
    {
	CodeGenOpt::Level OLvl = CodeGenOpt::Aggressive;
	EngineBuilder builder(getModule());
	builder.setErrorStr(&errormsg)
	    .setEngineKind(EngineKind::JIT)
	    .setOptLevel(OLvl);
	setCG(builder.create());
	if (!getCG()) {
	    errs() << "SKIRRuntime: error creating default code generator: ";
	    if (!errormsg.empty()) errs() << errormsg;
	    errs() << "\n";
	    assert(0);
	    //return false;
	}
    }
    
    class mylistener : public JITEventListener {
	std::map<void*, Function*> &fnMap;
    public:
	mylistener(std::map<void*, Function*> &fnMap) : fnMap(fnMap) {}
	virtual void NotifyFunctionEmitted(const Function &F,
					   void *FnStart, size_t FnSize,
					   const EmittedFunctionDetails &Details) {
	    fnMap[FnStart] = const_cast<Function*>(&F);
	    if (0) {
		//objdump -b binary -m i386-64 -D /tmp/skir_*.bin 
		outs() << "func@" << FnStart << " " << F.getNameStr() << " " << FnSize << "\n";
		std::fstream o(std::string("/tmp/skir_"+F.getNameStr()+".bin").c_str(),
			       std::fstream::out | std::fstream::trunc | std::fstream::binary);
		o.write((char*)FnStart,FnSize);
		o.close();
		
	    }
	}
	virtual void NotifyFunctionStubEmitted(const Function &F,
					       void *Addr, void *Stub) {
	    fnMap[Stub] = const_cast<Function*>(&F);
	    if (0)
		outs() << "stub@" << Stub << " " << F.getNameStr() << "\n";
	}
    };

    getCG()->RegisterJITEventListener(createOProfileJITEventListener());
    getCG()->RegisterJITEventListener(new mylistener(fn_map));
    getCG()->DisableLazyCompilation();

    sys::DynamicLibrary::LoadLibraryPermanently("/lib/libc.so.6");
    sys::DynamicLibrary::LoadLibraryPermanently("/usr/lib/libstdc++.so.6");
    //std::string skir_root(getenv("SKIR_ROOT"));
    //std::string filename(skir_root+"/skir/libinline_stream_ops.so");

    // runtime entry points
    typedef void (*vfp)(void);
    addSymbol("__SKIRRT_would_block", (vfp)__SKIRRT_would_block);
    addSymbol("__SKIRRT_kernel", (vfp)__SKIRRT_kernel);
    addSymbol("__SKIRRT_stream", (vfp)__SKIRRT_stream);
    addSymbol("__SKIRRT_array", (vfp)__SKIRRT_array);
    addSymbol("__SKIRRT_call", (vfp)__SKIRRT_call);
    addSymbol("__SKIRRT_wait", (vfp)__SKIRRT_wait);
    addSymbol("__SKIR_push", (vfp)__SKIR_push);
    addSymbol("__SKIR_pop", (vfp)__SKIR_pop);
    addSymbol("__SKIRRT_become", (vfp)__SKIRRT_become);

#if 0
    addSymbol("__SKIRRT_yield32", (vfp)__SKIRRT_yield32);
    addSymbol("__SKIRRT_return32", (vfp)__SKIRRT_return32);
#else
    addSymbol("__SKIRRT_yield64", (vfp)__SKIRRT_yield64);
    addSymbol("__SKIRRT_return64", (vfp)__SKIRRT_return64);
#endif

    // call before creating runtimegraph or schedulers
    // because both can create threads
    llvm_start_multithreaded();

    // create a stream graph
    setSG(new SKIRRuntimeGraph(*this));
    assert(getSG() && "Error creating Runtime Stream Graph!");
    getSG()->setVerbose(verbose);

    // create the default host scheduler
    setSched(getSG()->getTbbSched());
    //setSched(new SKIRMergeSched(getSG()));
    //setSched(new SKIRDPSched(getSG(), 2));

    assert(getSched() && "Error creating Tbb Scheduler!");
    getSched()->setVerbose(verbose);

    //getCG()->runStaticConstructorsDestructors(false);
    //getSched()->start();

    initialized = true;
}

//
// event handling
//
bool
SKIRRuntime::onEvent(std::stringstream *event)
{
    char buf[32];
    std::string type;
    bool die = false;

    event->getline(buf, 32);
    type.assign(buf);

    if (type.find("\r") != std::string::npos)
	type = type.substr(0,type.find("\r"));

    if (verbose)
	errs() << "onEvent: '" << type << "'\n";
    
    if (!type.compare("die")) {
	die = true;
    }

    else if (!type.compare("EchoRequest")) {
	EchoRequest echo;
	echo.ParseFromIstream(event);
	if (verbose) errs() << "echoing: " << echo.str() << "\n";

	RequestResponse ret;
	ret.set_request_id(echo.request_id());
	ret.set_data_string(echo.str());
	ret.set_type(RequestResponse::STRING);
 
	std::string response;
	if (!ret.SerializeToString(&response))
	    errs() << "CallRequest: failed to serialize response\n";
	event->str(response);

	return false;
    }	

    // include module
    else if (!type.compare("IncludeModuleRequest")) {
	IncludeModuleRequest req;
	req.ParseFromIstream(event);
	const char *c = req.mutable_str()->data();
	MemoryBuffer *buffer = MemoryBuffer::getMemBuffer(c, c+req.mutable_str()->length());
	SMDiagnostic Err;
	Module *mod = ParseAssembly(buffer, 0, Err, getModule()->getContext());
	if (mod) {
	    includeModule(mod);
	} else
	    Err.Print("includeAssembly", errs());

	RequestResponse ret;
	ret.set_request_id(req.request_id());
	ret.set_type(RequestResponse::OK);
 
	std::string response;
	if (!ret.SerializeToString(&response))
	    errs() << "CallRequest: failed to serialize response\n";
	event->str(response);

	return false;
    }

    // run from local file
    else if (!type.compare("RunModuleRequest")) {
	RunModuleRequest runModule;
	runModule.ParseFromIstream(event);

	std::vector< std::string > InputArgv;
	for (int i=0, j=runModule.input_argv_size(); i<j; i++) 
	    InputArgv.push_back(runModule.input_argv(i));

	char *const *envp = environ;
	run(runModule.input_module(),
	    InputArgv,
	    runModule.fake_argv0(),
	    runModule.entry_func(), 
	    envp,
	    runModule.nthreads(),
	    runModule.run_mode());

	die = true;
    }

    // execute a work function
    else if (!type.compare("CallRequest")) {
	CallRequest req;
	req.ParseFromIstream(event);
	event->str(std::string(""));

	// execute call instruction
	void *kernel = (void*)addr_map[req.kernel()];
	void **ins = new void*[req.ins_size()+1];
	void **outs = new void*[req.outs_size()+1];
	for (int i=0; i<req.ins_size(); i++) {
	    ins[i] = (void*)addr_map[req.ins(i)];
	    //std::cout << ins[i] << "\n";
	}
	for (int i=0; i<req.outs_size(); i++) {
	    outs[i] = (void*)addr_map[req.outs(i)];
	    //std::cout << outs[i] << "\n";
	}
	ins[req.ins_size()] = 0;
	outs[req.outs_size()] = 0;

	if (req.has_opt_only())
	    ((SKIRRuntimeKernel*)kernel)->opt_only = req.opt_only();

	handleCallInst(kernel, ins, outs);

	RequestResponse ret;
	ret.set_request_id(req.request_id());

	std::string response;
	if (!ret.SerializeToString(&response))
	    errs() << "CallRequest: failed to serialize response\n";
	event->str(response);

	return false;
    }

    // allocate a kernel
    else if (!type.compare("KernelRequest")) {
	KernelRequest req;
	req.ParseFromIstream(event);
	event->str(std::string(""));

	// find work and init functions
	Function *workF = getModule()->getFunction(req.work());
	Function *initF = getModule()->getFunction(req.init());
	//void *workfn = getCG()->getPointerToFunction(workF);
	//void *initfn = getCG()->getPointerToFunction(initF);
	
	if (!workF || !initF) {
	    std::stringstream ss;
	    if (!workF)
		ss << "KernelRequest: workfn '" << req.work() << "' not found\n";
	    if (!initF)
		ss << "KernelRequest: initfn '" << req.init() << "' not found\n";

	    if (verbose)
		errs() << ss.str();

	    RequestResponse ret;
	    ret.set_request_id(req.request_id());
	    ret.set_data_string(ss.str());
	    ret.set_type(RequestResponse::ERR);

	    std::string response;
	    if (!ret.SerializeToString(&response))
		errs() << "KernelRequest: failed to serialize response: \n\t" << ss;
	    event->str(response);
	    return false;
	}
	
	//printf("kernel req arg id %p\n", req.args());
	void *args = addr_map[ req.args() ];
	//printf("kernel req args %p\n", args);

	void *k = handleKernelInst(initF, workF, args);
	
	unsigned int id = req.request_id();
	addr_map[id] = k;

	RequestResponse ret;
	ret.set_request_id(id);
	ret.set_type(RequestResponse::UINT32);
	ret.set_data_uint32(id);

	std::string response;
	ret.SerializeToString(&response);
	event->str(response);

	return false;
    }

    // allocate a stream
    else if (!type.compare("StreamRequest")) {
	StreamRequest req;
	req.ParseFromIstream(event);
	event->str(std::string(""));

	void *s = handleStreamInst( req.size() );
	((SKIRRuntimeStream*)s)->type = req.type();
	if (req.has_id()) {
	    ((SKIRRuntimeStream*)s)->id = req.id();
	}

	unsigned int id = req.request_id();
	addr_map[id] = s;

	RequestResponse ret;
	ret.set_request_id(id);
	ret.set_type(RequestResponse::UINT32);
	ret.set_data_uint32(id);

	std::string response;
	ret.SerializeToString(&response);
	event->str(response);
	return false;
    }

    else if (!type.compare("ShmRequest")) {
	ShmRequest req;
	req.ParseFromIstream(event);
	event->str(std::string(""));

	unsigned int id = req.request_id();
	int shm_id = req.shm_id();
	size_t len = req.length();

	int fd;
	std::stringstream file_name;
	file_name << "/skir_state." << shm_id;
	fd = shm_open( file_name.str().c_str(), O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
	assert(fd >= 0);
	ftruncate(fd, len);

	char *data = (char*)mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	assert(data != MAP_FAILED);

	addr_map[shm_id] = data;
	//printf("new shm id %d = %p n=%d\n", shm_id, data, len);

	RequestResponse ret;
	ret.set_request_id(id);
	ret.set_type(RequestResponse::UINT32);
	ret.set_data_uint32(shm_id);

	std::string response;
	ret.SerializeToString(&response);
	event->str(response);
	return false;
    }

    // get/set kernel state
    else if (!type.compare("StateRequest")) {
	StateRequest req;
	req.ParseFromIstream(dynamic_cast<istream*>(event));
	event->str(std::string(""));

	unsigned int id = req.request_id();

	// put to here
	if (!req.has_type() || req.type() == StateRequest::PUT) {
	    assert(req.has_length());
	    size_t n = req.length();

	    void *s;
	    unsigned int data_id;

	    if (req.has_data_id()) {
		assert(!req.has_data_bytes());
		data_id = req.data_id();
		s = addr_map[data_id];
		//printf("lookup ");
	    } else {
		assert(req.has_data_bytes());
		data_id = id;
		int fd;
		std::stringstream file_name;
		file_name << "/skir_state." << id;
		fd = shm_open( file_name.str().c_str(), O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
		assert(fd >= 0);
		ftruncate(fd, n);

		s = (char*)mmap(0, n, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		assert(s != MAP_FAILED);

		const char *c = req.mutable_data_bytes()->data();
		memcpy(s, c, n);
		addr_map[data_id] = s;
		//printf("setting ");
	    }

	    //printf("data id %d = %p n=%d\n", data_id, s, n);

	    RequestResponse ret;
	    ret.set_request_id(id);
	    ret.set_type(RequestResponse::UINT32);
	    ret.set_data_uint32(data_id);
	
	    std::string response;
	    ret.SerializeToString(&response);
	    event->str(response);
	    return false;
	}
	else if (req.has_type() && req.type() == StateRequest::GET) {
	    assert(0);
	    // get from here
	    char *p = (char*)addr_map[ req.data_id() ];
	    size_t n = req.length();
	    std::string data_bytes(p, n);
	    
	    RequestResponse ret;
	    ret.set_request_id(id);
	    ret.set_type(RequestResponse::BYTES);
	    ret.set_data_bytes(data_bytes);
	
	    std::string response;
	    ret.SerializeToString(&response);
	    event->str(response);
	    return false;
	    
	} else {
	    // error
	    RequestResponse ret;
	    ret.set_request_id(id);
	    ret.set_type(RequestResponse::ERR);
	    std::stringstream ss;
	    ss << "bad request type";
	    ret.set_data_string(ss.str());
	    return false;
	}
    }


    // pause/unpause
    else if (!type.compare("PauseRequest")) {
	PauseRequest req;
	req.ParseFromIstream(event);
	event->str(std::string(""));
	
	if (req.pause())
	    getSG()->pauseKernel((SKIRRuntimeKernel *)req.kernel());
	else
	    getSG()->unPauseKernel((SKIRRuntimeKernel *)req.kernel());
    }

    // .
    else if (!type.compare("DotRequest")) {
	DotRequest req;
	req.ParseFromIstream(event);
	event->str(std::string(""));
    
	std::stringstream dot;
	getSG()->dot(dot);

	RequestResponse ret;
	ret.set_request_id(req.request_id());
	ret.set_data_string(dot.str());

	std::string response;
	ret.SerializeToString(&response);
	event->str(response);
	return false;
    }

    event->str(std::string(""));
    return die;
}

// 
// Run a SKIR program by calling the function 'EntryFunc' as main with arguments 'InputArgv' 
//  and 'envp'
// 
bool
SKIRRuntime::run(std::string InputFile, std::vector<std::string> InputArgv,
		 std::string FakeArgv0, std::string EntryFunc,
		 char *const *envp, int NThreads, int RunMode)
{
    initialize(NThreads);

    LLVMContext &CTX = getModule()->getContext();

    // Load the bitcode
    {
	std::string errormsg;
	Module *mod = 0;
	if (MemoryBuffer *buffer = MemoryBuffer::getFileOrSTDIN(InputFile, &errormsg)){
	    mod = ParseBitcodeFile(buffer, CTX, &errormsg);
	    delete buffer;
	}
	if (!mod) {
	    errs() << "SKIRRuntime: error loading program '" << InputFile << "': "
		   << errormsg << "\n";
	    return false;
	}
	includeModule(mod);
    }

    // If the user specifically requested an argv[0] to pass into the program,
    // do it now.
    if (!FakeArgv0.empty()) {
	InputFile = FakeArgv0;
    } else {
	// Otherwise, if there is a .bc suffix on the executable strip it off, it
	// might confuse the program.
	if (InputFile.rfind(".bc") == InputFile.length() - 3)
	    InputFile.erase(InputFile.length() - 3);
    }
    
    // Add the module's name to the start of the vector of arguments to main().
    InputArgv.insert(InputArgv.begin(), InputFile);

    // Call the main function from M as if its signature were:
    //   int main (int argc, char **argv, const char **envp)
    // using the contents of Args to determine argc & argv, and the contents of
    // EnvVars to determine envp.
    //
    Function *EntryFn = getModule()->getFunction(EntryFunc);
    if (!EntryFn) {
	errs() << '\'' << EntryFunc << "\' function not found in module.\n";
	return false;
    }
    
    // Reset errno to zero on entry to main.
    errno = 0;
 
    // Run static constructors.
    getCG()->runStaticConstructorsDestructors(false);

    // Run main.
    int Result = getCG()->runFunctionAsMain(EntryFn, InputArgv, NULL/*envp*/);

    // Run static destructors.
    getCG()->runStaticConstructorsDestructors(true);


    // If the program doesn't explicitly call exit, we will need the Exit 
    // function later on to make an explicit call, so get the function now. 
    Constant *Exit = getModule()->getOrInsertFunction("exit",
						      Type::getVoidTy(CTX),
						      Type::getInt32Ty(CTX),
						      NULL);
    
    // If the program didn't call exit explicitly, we should call it now. 
    // This ensures that any atexit handlers get called correctly.
    if (Function *ExitF = dyn_cast<Function>(Exit)) {
	std::vector<GenericValue> Args;
	GenericValue ResultGV;
	ResultGV.IntVal = APInt(32, Result);
	Args.push_back(ResultGV);
	getCG()->runFunction(ExitF, Args);
	errs() << "ERROR: exit(" << Result << ") returned!\n";
	abort();
    } else {
	errs() << "ERROR: exit defined with wrong prototype!\n";
	abort();
    }

    return true;
}

extern "C" 
{
    // void * @llvm.skir.kernel(void *runtime, void *init, void *work)
    void *
    __SKIRRT_kernel(void *me, void *init, void *work, void *args)
    {
	SKIRRuntime *rt = (SKIRRuntime *)me;
	return rt->handleKernelInst(init, work, args);
    }

    void 
    __SKIRRT_call(void *me, void *kernel, void *in_streams, void *out_streams)
    {
	SKIRRuntime *rt = (SKIRRuntime *)me;
	rt->handleCallInst(kernel, in_streams, out_streams);
    }

    void
    __SKIRRT_wait(void *me, void *kernel)
    {
	SKIRRuntime *rt = (SKIRRuntime *)me;
	rt->handleWaitInst(kernel);
    }


    void *
    __SKIRRT_stream(void *me, unsigned int elem_size)
    {
	SKIRRuntime *rt = (SKIRRuntime *)me;
	return rt->handleStreamInst(elem_size);
    }

    void *
    __SKIRRT_array(void *me, void *begin, void *end, unsigned int elem_size, unsigned int stride)
    {
	SKIRRuntime *rt = (SKIRRuntime *)me;
	return rt->handleArrayInst(begin, end, elem_size, stride);
    }

    void
    __SKIRRT_become(void *me, void *k, void *ins, void *outs)
    {
	SKIRRuntime *rt = (SKIRRuntime *)me;
	rt->handleBecomeInst(k, ins, outs);
    }

    void
    __SKIRRT_would_block(void *, void*)
    {
	assert(0 && "error: __SKIRRT_would_block called");
    }

    void*
    __SKIRRT_workfn_extern(skir_rt_state_t   *rt_state, 
			   void              *kernel_state, 
			   skir_stream_t *ins[],
			   skir_stream_t *outs[])
    {
	assert(0 && "error: ____SKIRRT_workfn_extern called");
	return (void*)0;
    }

    void*
    __SKIRRT_workfn_extern0(skir_rt_state_t   *rt_state, 
			   void              *kernel_state, 
			   skir_stream_t *ins[],
			   skir_stream_t *outs[])
    {
	assert(0 && "error: ____SKIRRT_workfn_extern0 called");
	return (void*)0;
    }

    void*
    __SKIRRT_workfn_extern1(skir_rt_state_t   *rt_state, 
			   void              *kernel_state, 
			   skir_stream_t *ins[],
			   skir_stream_t *outs[])
    {
	assert(0 && "error: ____SKIRRT_workfn_extern1 called");
	return (void*)0;
    }

    void
    __SKIR_push(skir_stream_idx_t p, skir_stream_element_t e)
    {
	outs() << "STUB:  " << __func__ << " " << e << "\n";
	assert(0);
    }

    void
    __SKIR_pop(skir_stream_idx_t p, skir_stream_element_t e)
    {
	outs() << "STUB:  " << __func__ << "\n";
	assert(0);
    }

    void
    __SKIR_ins_commit(skir_stream_ptr_t in_streams[])
    {
	outs() << "STUB:  " << __func__ << "\n";
	assert(0);
    }

    void
    __SKIR_outs_commit(skir_stream_ptr_t out_streams[])
    {
	outs() << "STUB:  " << __func__ << "\n";
	assert(0);
    }

    unsigned long long
    __SKIR_rdtsc(void) 
    {
	outs() << "STUB:  " << __func__ << "\n";
	assert(0);
    }    

    void
    __SKIR_prefetch(void *p, int rw, int l) 
    {
	outs() << "STUB:  " << __func__ << "\n";
	assert(0);
    }    

} // extern "C"
