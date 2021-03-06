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

#ifndef _SKIR_MERGE_SCHED_H_
#define _SKIR_MERGE_SCHED_H_

#include "SKIRScheduler.h"
#include "SKIRRuntimeKernel.h"

#include <tbb/atomic.h>
#include <tbb/tbb_thread.h>
#include <tbb/concurrent_queue.h>

#include <set>
#include <vector>

namespace llvm {

// the merge scheduler greedily fuses tasks as they are called
// merging stops and execution begins at the first wait operation

class SKIRMergeSched : public SKIRScheduler {
public:

    SKIRMergeSched(SKIRRuntimeGraph *stream_graph) 
	: sg(stream_graph), verbose(false)
    {
	running = 0;
	sched = sg->getTbbSched();
    }

    void callKernel(SKIRRuntimeKernel *rtk);
    void removeKernel(SKIRRuntimeKernel *rtk);
    void waitKernel(SKIRRuntimeKernel *rtk);
    void pauseKernel(SKIRRuntimeKernel *rtk);
    void unPauseKernel(SKIRRuntimeKernel *rtk);

    void runCodeGen(SKIRRuntimeKernel *rtk);

    void start(void);
    void stop(void);
    void run(void);

    void setVerbose(bool v) { verbose = v; }

 private:

    SKIRRuntimeKernel *tryMerge(SKIRRuntimeKernel *rtk);

    SKIRRuntimeGraph *sg;
    SKIRScheduler *sched;
    bool verbose;

    tbb::tbb_thread *main_thread;    
    tbb::atomic<int> running;
    tbb::concurrent_bounded_queue<SKIRRuntimeKernel *> runq;

    typedef std::set<SKIRRuntimeKernel *> kernel_set_t;
    typedef std::pair< SKIRRuntimeKernel*, kernel_set_t > merge_set_t;
    typedef std::vector< merge_set_t* > merge_sets_t;

    merge_sets_t merge_sets;

    merge_set_t *kernel_map_find(SKIRRuntimeKernel *rtk);
    void kernel_map_erase(merge_set_t *m);
    void kernel_map_append(merge_set_t *m);
    void kernel_map_insert(SKIRRuntimeKernel *rtk, merge_set_t *m);
    void kernel_map_replace(SKIRRuntimeKernel *k, merge_set_t *m);
};

}
#endif //_SKIR_MERGE_SCHED_H_
