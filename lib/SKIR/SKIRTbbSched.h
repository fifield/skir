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

#ifndef _SKIR_TBB_SCHED_H_
#define _SKIR_TBB_SCHED_H_

#include "SKIRScheduler.h"

#include <tbb/task.h>
#include <tbb/atomic.h>
#include <tbb/tbb_thread.h>
#include <tbb/concurrent_queue.h>

namespace llvm {

class SKIRRuntimeGraph;
class kernel_t;

class SKIRTbbSched : public SKIRScheduler {
public:
    SKIRTbbSched(SKIRRuntimeGraph *stream_graph, int nthreads);

    void callKernel(SKIRRuntimeKernel *rtk);
    void removeKernel(SKIRRuntimeKernel *rtk);
    void waitKernel(SKIRRuntimeKernel *rtk);

    void pauseKernel(SKIRRuntimeKernel *rtk);
    void unPauseKernel(SKIRRuntimeKernel *rtk);

    void runCodeGen(SKIRRuntimeKernel *rtk);

    void start(void);
    void stop(void);

    void setAffinities(void);

    void setVerbose(bool v) { verbose = v; }
    
    void *runDPWork(void *rt_state, void *state, void *i, void *o);

    void run(void);

    int loadCallback(float load);

private:
    SKIRRuntimeGraph *sg;

    int cur_workers;
    int num_workers;
    bool verbose;

    tbb::task *root_task;
    tbb::tbb_thread *main_thread;
    tbb::tbb_thread *mon_thread;

    tbb::atomic<int> running;

    tbb::concurrent_bounded_queue<kernel_t *> runq;


};

}
#endif
