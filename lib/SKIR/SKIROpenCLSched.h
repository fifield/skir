#ifndef _SKIR_OPENCL_SCHED_H_
#define _SKIR_OPENCL_SCHED_H_

#include "SKIRScheduler.h"
#include "SKIRRuntimeGraph.h"

#include <string>
#include <tbb/atomic.h>

namespace llvm {

class SKIRRuntime;

#ifdef USE_OPENCL
class SKIROpenCLSched : public SKIRScheduler
{
public:

    SKIROpenCLSched(SKIRRuntimeGraph *stream_graph);
    ~SKIROpenCLSched();

    void callKernel(SKIRRuntimeKernel *rtk);
    void removeKernel(SKIRRuntimeKernel *rtk);
    void waitKernel(SKIRRuntimeKernel *rtk);
    
    void pauseKernel(SKIRRuntimeKernel *rtk);
    void unPauseKernel(SKIRRuntimeKernel *rtk);

    void runOpenCLBackend(SKIRRuntimeKernel *rtk, std::string &output);
    void runCodeGen(SKIRRuntimeKernel *rtk);

    void start(void);
    void stop(void);

    Module *getCLModule();

    void setVerbose(bool v) { verbose = v; }

private:

    void setCLModule(Module *mod) { opencl_module = mod; }
    void replaceUsesOfType(Function *F, const Type *oldTy, const Type *newTy);
    const Type *getStateType(Value *statePtr);

    // data

    SKIRRuntimeGraph *sg;
    SKIRScheduler *the_single_sched;

    bool verbose;
    tbb::atomic<int> running;
    Module *opencl_module;

};
#endif //USE_OPENCL

}
#endif

