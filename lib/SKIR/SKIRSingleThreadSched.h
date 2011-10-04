#ifndef _SKIR_SINGLE_THREAD_SCHED_H_
#define _SKIR_SINGLE_THREAD_SCHED_H_

#include "SKIRScheduler.h"
#include "SKIRRuntimeKernel.h"
#include "SKIR_kernel_t.h"

#include <tbb/atomic.h>
#include <tbb/tbb_thread.h>
#include <tbb/concurrent_queue.h>

namespace llvm {

class SKIRSingleThreadSched : public SKIRScheduler {
public:
    SKIRSingleThreadSched(SKIRRuntimeGraph *stream_graph) : sg(stream_graph),
							    verbose(false)
    {
	running = 0;
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
    void empty(void);

    void setVerbose(bool v) { verbose = v; }

 private:
    
    SKIRRuntimeGraph *sg;
    bool verbose;

    // hash_map<int> kernel_map 
    kernel_map_t kernel_map;
    void kernel_map_insert(SKIRRuntimeKernel *key, kernel_t *value);
    kernel_t *kernel_map_find(SKIRRuntimeKernel *key);

    tbb::tbb_thread *main_thread;    
    tbb::atomic<int> running;
    tbb::concurrent_bounded_queue<SKIRRuntimeKernel *> runq;  
};

}
#endif
