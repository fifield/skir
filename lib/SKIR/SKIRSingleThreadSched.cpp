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

#include <llvm/Support/raw_ostream.h>

#include "SKIR/SKIRRuntime.h"
#include "SKIRRuntimeGraph.h"
#include "SKIRSingleThreadSched.h"
#include "SKIR_kernel_t.h"
//#include "inline_stream_ops.h"

namespace llvm {

// kernel_map[key]
kernel_t *
SKIRSingleThreadSched::kernel_map_find(SKIRRuntimeKernel *key)
{
    kernel_map_t::const_accessor a;
    if (kernel_map.find(a,key))
	return a->second;
    return NULL;
}
    
// kernel_map[key] = value
void
SKIRSingleThreadSched::kernel_map_insert(SKIRRuntimeKernel *key, kernel_t *value)
{
    kernel_map_t::accessor a;
    if (!kernel_map.find(a,key))
	kernel_map.insert(a, key);
    a->second = value;
}

void
SKIRSingleThreadSched::callKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = 0;
    k = new kernel_t(*rtk);
    assert(k);
    kernel_map_insert(rtk, k);
    k->active();
    runq.push(rtk);
    start();
}

void
SKIRSingleThreadSched::waitKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    assert(k);
    k->wait(DONE);
}

void
SKIRSingleThreadSched::pauseKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    assert(k);
    k->pause();
}

void
SKIRSingleThreadSched::unPauseKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    assert(k);
    k->unpause();
}

void
SKIRSingleThreadSched::runCodeGen(SKIRRuntimeKernel *rtk)
{
    if (rtk->fpm)
	delete rtk->fpm;

    rtk->fpm = new FunctionPassManager(rtk->work->getParent());
    rtk->fpm->add(new TargetData(rtk->work->getParent()));

    SKIRRuntime::addSKIRBlockingOpsPass(rtk);
    SKIRRuntime::addSKIRInlineStreamsPass(rtk);
    SKIRRuntime::addLLVMOpts(rtk);

    sg->codeGenKernel(rtk);
    assert(rtk->workfn);
}

struct skir_single_thread {
    SKIRSingleThreadSched &s;
    skir_single_thread(SKIRSingleThreadSched &sched) : s(sched) {}
    void operator()() { s.run(); }
};

void
SKIRSingleThreadSched::start(void)
{
    if (running.compare_and_swap(1,0) == 0) {
	main_thread = new tbb::tbb_thread(*(new skir_single_thread(*this)));
	assert(main_thread);
    }
}

void
SKIRSingleThreadSched::stop(void)
{
    if (!main_thread) return;

    if (running.compare_and_swap(0,1) == 1) {
	assert(main_thread && main_thread->joinable() && "not running");
	main_thread->join();
	main_thread = NULL;
    }

    SKIRRuntimeKernel *k;
    while (runq.try_pop(k))
	removeKernel(k);
}

void
SKIRSingleThreadSched::empty(void)
{
    if (running.compare_and_swap(0,1) == 1) {

    }
	
}

// stop executing rtk, and remove internal state associated with it
void
SKIRSingleThreadSched::removeKernel(SKIRRuntimeKernel *rtk)
{
    kernel_t *k = kernel_map_find(rtk);
    if (!k) return;

    // pause (lock) the kernel so it won't get picked for execution
    k->pause();

    // remove the kernel from the internal list
    kernel_map_insert(rtk, NULL);

    // release the pause thread
    k->done();
    
    // once we can get the lock, it's safe to delete k
    {
	kernel_lock_t::scoped_lock lock;
	lock.acquire(k->lock);
	k->running = 0;
    }
    
    delete k;
}

void
SKIRSingleThreadSched::run()
{
    while (running == 1)
    {
	kernel_t *k = 0;
	SKIRRuntimeKernel *rtk = 0;
	if (runq.try_pop(rtk)) {
	    // make sure k doesn't reference garbage (e.g. removed kernel)
	    k = kernel_map_find(rtk);
	    if (!k) continue;

	    kernel_lock_t::scoped_lock lock;
	    if (lock.try_acquire(k->lock)) {
		if (k->is_active()) {
		    // generate workfn if needed
		    if (!k->rt_kernel.workfn) {
			k->rt_kernel.sched->runCodeGen(&k->rt_kernel);
			assert(k->rt_kernel.workfn);
		    }
		    
		    // execute work function until it returns non-zero
		    SKIRRuntimeKernel *b;
		    do {
			b = k->work();
		    } while (b == 0);

		    if (b == (SKIRRuntimeKernel *)1)
			k->done();
		}
	    } else {
		// if there's only one kernel in the scheduler,
		// we'll only not get the lock if the kernel is paused,
		// so we should yield if this happens
		if (runq.empty()) tbb::this_tbb_thread::yield();
	    }

	    if (!k->is_done())
		runq.push(rtk);
	} else {
	    // runq is empty, yield
	    //tbb::this_tbb_thread::yield();
	    empty();
	    
	}

    } // while (running == 1)
}


}//namespace llvm
