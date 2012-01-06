//===----------------------------------------------------------------------===//
// Copyright (c) 2011 Regents of the University of Colorado 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to 
// deal in the Software without restriction, including without limitation the 
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is 
// furnished to do so, subject to the following conditions: 
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software. 
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE. 
//===----------------------------------------------------------------------===//

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

