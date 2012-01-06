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

#ifndef _SKIR_KERNEL_T_H_
#define _SKIR_KERNEL_T_H_

#include "SKIRRuntimeKernel.h"
#include "SKIRTiming.h"
#include <pthread.h>
#include <tbb/tbb.h>

//
// kernel_t - an internal representation for kernels
//
namespace llvm {

struct kernel_t;

typedef tbb::spin_mutex kernel_lock_t;

class kernel_task;

typedef enum {
    IDLE,
    ACTIVE,
    PAUSED,
    DONE
} kernel_state_t;

struct pause_tbb_thread;

struct kernel_t {
    kernel_t(SKIRRuntimeKernel &k) :
	running(), owning_task(NULL), lock(), rt_kernel(k)
    {
	running = 0;
	retries = 0;
	fail = 0;
	id = k.id;
	wait_cnt = 0;
	pthread_mutex_init(&wait_mutex, NULL);
	pthread_cond_init (&wait_cond, NULL);
	wait_state = IDLE;
	niter_cb = 0;
	d4r_cb = 0;
    }

    ~kernel_t() {
	wait_state = IDLE;
	pthread_mutex_destroy(&wait_mutex);
	pthread_cond_destroy(&wait_cond);
    }

    SKIRRuntimeKernel* work() {
	rt_kernel.rt_state->niter = 0;
	unsigned long long tsc;
	rdtscll(tsc);
	SKIRRuntimeKernel *r = (SKIRRuntimeKernel *)rt_kernel.workfn((void*)rt_kernel.rt_state,
								     (void*)rt_kernel.state,
								     (void*)rt_kernel.impl_ins,
								     (void*)rt_kernel.impl_outs);
	unsigned long long tsc2;
	rdtscll(tsc2);
	rt_kernel.rt_state->cycles += (tsc2 - tsc);
	rt_kernel.total_runtime += (tsc2 - tsc);
	rt_kernel.total_niter += rt_kernel.rt_state->niter;
	rt_kernel.total_ncall++;

        if (d4r_cb) {
            if ((rt_kernel.total_ncall & 0xff) == 0) {
                d4r_cb(d4r_cb_data, &rt_kernel, r);
            }
        }
	if (niter_cb) {
	    if ((rt_kernel.total_niter & 0x3) == 0) {
		r = niter_cb(niter_cb_data, &rt_kernel, r);
	    }
	}

	return r;
    }

    // these are the states we can be in
    // we always signal on a change of state
    void idle() {
	wait_state = IDLE;
    }

    void active() {
	pthread_mutex_lock(&wait_mutex);
	//if (is_done()) {
	//pthread_mutex_unlock(&wait_mutex);
	//return;
	//}
	wait_state = ACTIVE;
	pthread_cond_signal(&wait_cond);
	pthread_mutex_unlock(&wait_mutex);
    }
    bool is_active() {
	return (wait_state == ACTIVE);
    }

    void done()
    {
	// signal done
	//
	pthread_mutex_lock(&wait_mutex);

	rt_kernel.done();

	wait_state = DONE;
	pthread_cond_signal(&wait_cond);

	pthread_mutex_unlock(&wait_mutex);

	//errs() << rt_kernel.work->getName() << " DEAD\n";
    }
    bool is_done() {
	return (wait_state == DONE);
    }

    // wait for given state (or DONE)
    void wait(kernel_state_t state) {
	pthread_mutex_lock(&wait_mutex);
	wait_cnt++;
	while (wait_state != state) {
	    if (is_done()) break;
	    pthread_cond_wait(&wait_cond, &wait_mutex);
	}
	wait_cnt--;
	pthread_mutex_unlock(&wait_mutex);
    }

    // this simply spawns a thread that waits until the kernel is active
    // while it waits, it holds the kernel's lock, preventing it from executing.
    struct pause_thread {
	kernel_t *kern;
	pause_thread(kernel_t *kernel) : kern(kernel) { }
	void operator()() {
	    if (kern->is_paused()) {
		kernel_lock_t::scoped_lock lock;
		lock.acquire(kern->lock);
		kern->wait(ACTIVE);
	    }
	}
    };

    bool is_paused() {
	return (wait_state == PAUSED);
    }
    void pause() {
	pthread_mutex_lock(&wait_mutex);
	if (is_done()) {
	    pthread_mutex_unlock(&wait_mutex);
	    return;
	}
	wait_state = PAUSED;
	pthread_cond_signal(&wait_cond);
	pthread_mutex_unlock(&wait_mutex);

	new tbb::tbb_thread(*(new pause_thread(this)));
    }

    void unpause() {
	active();
    }

    // these can only be written if lock is held
    tbb::atomic<int> running;
    // end locked section

    //kernel_task *owner;
    tbb::task *owning_task;
    kernel_lock_t lock;

    // state shared with SKIRRuntimeKernel
    SKIRRuntimeKernel &rt_kernel;

    int retries;
    //int retries_base;
    int fail;

    unsigned id;

    // skir.wait support
    kernel_state_t wait_state;
    pthread_mutex_t wait_mutex;
    pthread_cond_t wait_cond;
    int wait_cnt;

    SKIRRuntimeKernel* (* niter_cb)(void *, SKIRRuntimeKernel *me, SKIRRuntimeKernel *ret);
    void *niter_cb_data;

    SKIRRuntimeKernel* (* d4r_cb)(void *, SKIRRuntimeKernel *me, SKIRRuntimeKernel *ret);
    void *d4r_cb_data;

    struct task_stats {
	task_stats() {
	    num_work_calls = 0;
	    num_cycles = 0;
	    num_steal = 0;
	    num_retries = 0;
	    num_continue = 0;
	    num_tasks = 0;
	    num_fail = 0;
	}
	unsigned long long num_work_calls;
	unsigned long long num_cycles;
	unsigned long long num_steal;
	unsigned long long num_retries;
	unsigned long long num_continue;
	unsigned long long num_tasks;
	unsigned long long num_fail;
    } stats;

};


struct hash_compare {
    static size_t hash(SKIRRuntimeKernel * const& key) {
	return reinterpret_cast<size_t>(key)/sizeof(key);
    }
    static bool equal(SKIRRuntimeKernel * const& a, SKIRRuntimeKernel * const& b) {
	return a == b;
    }
};
typedef tbb::concurrent_hash_map<SKIRRuntimeKernel *, kernel_t *, hash_compare> kernel_map_t;

}

#endif
