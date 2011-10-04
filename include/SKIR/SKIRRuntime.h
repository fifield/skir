#ifndef _SKIR_RUNTIME_H_
#define _SKIR_RUNTIME_H_

#include <vector>
#include <map>
#include <list>
#include <string>
#include <iostream>
#include <sstream>

#include <llvm/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Function.h>
#include <llvm/Support/MutexGuard.h>

#include <SKIR/SKIRStream.h>

#define USE_TBB 1
#if USE_TBB
#include <tbb/atomic.h>
#endif

namespace llvm {

class SKIRRuntimeGraph;
class SKIRRuntimeStream;
class SKIRRuntimeKernel;
class SKIRScheduler;
 
class SKIRRuntime {

public:    
    // program entry point
    bool run(std::string InputFile, std::vector<std::string> InputArgv,
	     std::string FakeArgv0, std::string EntryFunc,
	     char *const *envp, int NThreads, int RTFlag);

    // init without running anything
    void start(int nthreads) { initialize(nthreads); }

    // add symbol
    typedef void (*vfp)(void);
    void addSymbol(const char *name, vfp symbol);

    // instruction entry points
    void *handleKernelInst(void *init, void *work, void *args);
    void *handleKernelInst(Function *init, Function *work, void *args);
    void handleCallInst(void *kernel, void *ins, void *outs);
    void handleWaitInst(void *kernel);
    void handleBecomeInst(void *kernel, void *ins, void *outs);
    void *handleStreamInst(unsigned elem_size);
    void *handleArrayInst(void *begin, void *end, unsigned elem_size, unsigned stride);

    // event handlers
    bool onEvent(std::stringstream *event);

    SKIRRuntime();
    ~SKIRRuntime();

    void setVerbose(bool v);
    bool getVerbose(void) { return verbose; }

    // return the module/code gen/stream graph used for 
    // storing/generating/executing code on the host
    Module *getModule() { return host_mod; }
    ExecutionEngine* getCG() { return host_cg; }
    SKIRRuntimeGraph* getSG() { return host_sg; }
    SKIRScheduler* getSched() { return host_sched; }
 
    unsigned getNumThreads() { return num_threads; }

    // skir program and kernel transformation passes
    static void runSKIRLoweringPasses(SKIRRuntime *p, Module *m);
    static void runSKIRCloneWorkPass(SKIRRuntimeKernel *k, Module *dstMod=0);
    static void runSKIRAddReentriesPass(SKIRRuntimeKernel *k);
    static void runSKIROuterLoopPass(SKIRRuntimeKernel *k, char *, char *);
    static void runSKIRRemovePointerArgsPass(SKIRRuntimeKernel *k);
    static void runSKIRInlineStreamsPass(SKIRRuntimeKernel *k);
    static void runSKIRKernelInfoPass(SKIRRuntimeKernel *K);
    static void runSKIRStreamsToArraysPass(SKIRRuntimeKernel *K);
    static void runLLVMOpts(SKIRRuntimeKernel *k);
    static void runSKIRBlockingOpsPass(SKIRRuntimeKernel *k);
    static void runSKIRStreamOptsPass(SKIRRuntimeKernel *k);
    static void runSKIRKoroPass(SKIRRuntimeKernel *k);

    static void addSKIRAddReentriesPass(SKIRRuntimeKernel *k);
    static void addSKIROuterLoopPass(SKIRRuntimeKernel *k, char *, char *);
    static void addSKIRRemovePointerArgsPass(SKIRRuntimeKernel *k);
    static void addSKIRInlineStreamsPass(SKIRRuntimeKernel *k);
    static void addSKIRKernelInfoPass(SKIRRuntimeKernel *K);
    static void addSKIRStreamsToArraysPass(SKIRRuntimeKernel *K);
    static void addLLVMOpts(SKIRRuntimeKernel *k);
    static void addSKIRBlockingOpsPass(SKIRRuntimeKernel *k);
    static void addSKIRStreamOptsPass(SKIRRuntimeKernel *k);
    static void addSKIRKoroPass(SKIRRuntimeKernel *k);

    int nextKernelID() { return next_kernel_id++; }

 private:
    void initialize(int nthreads);

    void setModule(Module *mod) { host_mod = mod; }
    void setCG(ExecutionEngine *cg) { host_cg = cg; }
    void setSG(SKIRRuntimeGraph *sg) { host_sg = sg; }
    void setSched(SKIRScheduler *sched) { host_sched = sched; }
    void setNumThreads(unsigned nthreads) { num_threads = nthreads; }

    void runInitFunction(SKIRRuntimeKernel *k, void *args);

    SKIRRuntimeStream *newStream();
    SKIRRuntimeKernel *newKernel();
    
    SKIRRuntimeKernel *mergeKernels(SKIRRuntimeKernel *K1, SKIRRuntimeKernel *K2);
    
    // analysis passes
    void computeRates(Function*& F);

    // add bitcode to host_mod
    void includeModule(Module *mod);

    // private data
    bool initialized;

    unsigned next_stream_id;
    unsigned next_kernel_id;

    // default architecture, code generator, module, and scheduler
    Module *host_mod;
    ExecutionEngine *host_cg;
    SKIRRuntimeGraph *host_sg;
    SKIRScheduler *host_sched;

    int num_threads;

    // map of generated code to llvm code
    std::map<void*, Function*> fn_map;

    // map a id/hash to an address
    // used by event handler: void* addr == addr_map[unsigned int request_id]
    std::map<unsigned int, void*> addr_map;

    bool verbose;
};

}
#endif
