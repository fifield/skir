#ifndef _SKIR_DP_SCHED_H_
#define _SKIR_DP_SCHED_H_

#include <SKIR/SKIRRuntime.h>
#include "SKIRRuntimeGraph.h"
#include "SKIRScheduler.h"
#include "SKIRRuntimeKernel.h"
#include "SKIR_kernel_t.h"

#include <tbb/atomic.h>
#include <tbb/tbb_thread.h>
#include <tbb/concurrent_queue.h>

#include <set>
#include <vector>

namespace llvm {

///
/// The SKIRDPSched implements dynamic fission.
///
class SKIRDPSched : public SKIRScheduler {
public:

    SKIRDPSched(SKIRRuntimeGraph *stream_graph, int width) 
	: sg(stream_graph), width(width)
    {
	verbose = false;
	running = 0;
	sched = sg->getTbbSched();
	my_kernel = 0;
	my_split = my_join = 0;
	next_dp_sched = 0;
    }

    void callKernel(SKIRRuntimeKernel *rtk);

    void removeKernel(SKIRRuntimeKernel *rtk) {
	sched->removeKernel(rtk);
    }

    void waitKernel(SKIRRuntimeKernel *rtk) { 
	if (rtk == my_kernel)
	    sched->waitKernel(my_join);
	else
	    sched->waitKernel(rtk);
    }

    void pauseKernel(SKIRRuntimeKernel *rtk) { 
	if (rtk == my_kernel)
	    sched->pauseKernel(my_split);
	else
	    sched->pauseKernel(rtk);
    }

    void unPauseKernel(SKIRRuntimeKernel *rtk) { 
	if (rtk == my_kernel)
	    sched->unPauseKernel(my_split);
	else
	    sched->unPauseKernel(rtk);
    }

    void runCodeGen(SKIRRuntimeKernel *rtk);

    void start(void) { 
	sched->start();
    }
    void stop(void) {
    }
    void run(void) {
    }

    void setVerbose(bool v) { 
	verbose = v;
    }

 private:

    SKIRRuntimeGraph *sg;
    SKIRRuntimeKernel *my_kernel;
    std::set<SKIRRuntimeKernel*> dp_kernels;
    SKIRRuntimeKernel *my_split, *my_join;
    SKIRScheduler *sched, *next_dp_sched;
    bool verbose;
    int width;
    
    tbb::atomic<int> running;
};

}
#endif //_SKIR_DP_SCHED_H_
