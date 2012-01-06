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

#ifndef _SKIR_RUNTIME_KERNEL_H_
#define _SKIR_RUNTIME_KERNEL_H_

#include <llvm/Function.h>
#include <llvm/PassManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <SKIR/SKIRStream.h>
#include "SKIRRuntimeStream.h"

#include "D4RTag.h"

//#include <tbb/atomic.h>
#include <tbb/spin_mutex.h>

#include <string.h>
#include <vector>

namespace llvm {

typedef void* work_function(void*, void*, void *, void *);

class SKIRScheduler;

// for D4R
typedef tbb::spin_mutex tag_lock_t;

// representation of kernels in the runtime grpah
// NB: if location of rt_state changes, the coroutine impl must change (e.g. SKIRRT_yield)
struct SKIRRuntimeKernel
{
    // host executable code for work
    work_function *workfn;

    // runtime defined state 
    skir_rt_state_t *rt_state;
    
    // programmer defined state
    void *state;

    // arrays of input and output streams (skir_stream_t)
    void **impl_ins;
    void **impl_outs;
    
    // unique kernel identifier
    unsigned id;

    // original llvm code for work fn
    Function *base_work;

    // possibly transformed llvm code for work fn
    Function *work;

    // programmer defined input and output streams (arguments to skir.call)
    SKIRRuntimeStream **rt_ins;
    SKIRRuntimeStream **rt_outs;
    SKIRRuntimeStream **rt_ints; // streams internal to a fused kernel

    // currently used by DPSched to save rt_ins and rt_outs
    SKIRRuntimeStream **sched_ins;
    SKIRRuntimeStream **sched_outs;

    // number of items in the above arrays
    int nins;
    int nouts;
    int nints;

    // don't run analysis routines if set
    bool opt_only;

    // flags set by SKIRKernelInfoPass
    // if setting opt_only == true, then these should be set manually
    bool is_hier;
    bool is_stateful;
    bool is_fixed_rate;
    bool is_const_idx;
    bool has_push;
    bool has_pop;
    bool has_peek;
    
    // child kernels if is_heir
    std::vector<SKIRRuntimeKernel*> children;

    // passes to run on kernel before codegen
    FunctionPassManager *fpm;

    // codegen responsible for generating this kernel
    ExecutionEngine *cg;

    // sched responsible for executing this kernel
    SKIRScheduler *sched;
    bool fixed_sched; // if true, don't automatically assign sched

    int affinity;

    // for D4R
    SKIRRuntimeKernel *last_blocker;
    size_t last_niter;
    D4R::Tag publicTag;
    D4R::Tag privateTag;
    tag_lock_t taglock;

    // stats
    unsigned long long total_runtime;
    unsigned long long total_niter;
    unsigned long long total_ncall;
    unsigned long long total_bytes;
    unsigned long long total_jit_time;
    unsigned long long total_runtime_per_thread[48];

    // constructor
    SKIRRuntimeKernel(unsigned i) : publicTag(i), privateTag(i), taglock()
    {
	workfn = NULL;
	state = NULL;
	impl_ins = NULL;
	impl_outs = NULL;

	rt_state = new skir_rt_state_t;
	memset(rt_state, 0, sizeof(skir_rt_state_t));

	id = i;

	base_work = NULL;
	work = NULL;

	rt_ins = NULL;
	rt_outs = NULL;
	rt_ints = NULL;
	sched_ins = NULL;
	sched_outs = NULL;

	nins = 0;
	nouts = 0;
	nints = 0;

	opt_only = false;

	is_hier = false;
	is_stateful = true;
	is_fixed_rate = false;
	is_const_idx = false;
	has_push = false;
	has_peek = false;
	has_pop = false;

	sched = 0;
	fixed_sched = false;

	fpm = 0;
	cg = 0;
	
	affinity = 0;

	// stats
	total_runtime = 0;
	total_jit_time = 0;
	total_niter = 0;
	total_ncall = 0;
	total_bytes = 0;
	for (int i=0; i<48; i++) {
	    total_runtime_per_thread[i] = 0;
	}
    }

    void done() 
    {
	std::vector<SKIRRuntimeKernel*>::iterator I,E;
	for (I=children.begin(), E=children.end(); I!=E; ++I) {
	    SKIRRuntimeKernel *child = *I;
	    child->done();
	}

	// deallocate stream state
	//
	SKIRRuntimeStream **r_ins = rt_ins;
	SKIRRuntimeStream **r_outs = rt_outs;
	void **i_ins = impl_ins;
	void **i_outs = impl_outs;

	impl_ins = 0;
	impl_outs = 0;
	
	for (int i=0; i<nins && i_ins; i++) {
	    skir_stream_t *s = (skir_stream_t*)i_ins[i];
	    if (s) {
		SKIRRuntimeStream *rs = (SKIRRuntimeStream *)s->rs;
		assert(rs == r_ins[i] || rs->type == SKIRRuntimeStream::SHARED);

		i_ins[i] = 0;
		s->dst = (void *)1;
		total_bytes += (s->num_push * s->elem_size);
		free_skir_stream_t(s);
	    }
	}

	for (int i=0; i<nouts && i_outs; i++) {
	    skir_stream_t *s = (skir_stream_t *)i_outs[i];
	    if (s) {
		SKIRRuntimeStream *rs = (SKIRRuntimeStream *)s->rs;

		assert(rs == r_outs[i] || rs->type == SKIRRuntimeStream::SHARED);

		i_outs[i] = 0;
		s->src = (void *)1;
		total_bytes += (s->num_push * s->elem_size);
		free_skir_stream_t(s);
	    }
	}

	//delete rt_state;
	
	//if (r_ins) delete[] r_ins;
	//if (r_outs) delete[] r_outs;
	if (i_ins) delete[] i_ins;
	if (i_outs) delete[] i_outs;
    }

private:
    SKIRRuntimeKernel() {}
    SKIRRuntimeKernel(const SKIRRuntimeKernel &k) {}

};

}
#endif
