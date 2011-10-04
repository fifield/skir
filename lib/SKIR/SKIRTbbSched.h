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

private:
    SKIRRuntimeGraph *sg;

    int num_workers;
    bool verbose;

    tbb::task *root_task;
    tbb::tbb_thread *main_thread;

    tbb::atomic<int> running;

    tbb::concurrent_bounded_queue<kernel_t *> runq;


};

}
#endif
