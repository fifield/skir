#include "SKIR/SKIRRuntime.h"
#include "SKIRRuntimeGraph.h"
#include "SKIRDPSched.h"
#include "SKIRUtil.h"
#include "inline_stream_ops.h"
#include "SKIRSingleThreadSched.h"
#include "SKIROpenCLSched.h"

using namespace llvm;

void
SKIRDPSched::runCodeGen(SKIRRuntimeKernel *rtk)
{
    // pass everything but the split and the
    // join to the underlying scheduler
    if ((rtk != my_split) && (rtk != my_join))
	sched->runCodeGen(rtk);

    if (rtk->fpm)
	delete rtk->fpm;

    rtk->fpm = new FunctionPassManager(rtk->work->getParent());
    rtk->fpm->add(new TargetData(rtk->work->getParent()));

    SKIRRuntime::addSKIRInlineStreamsPass(rtk);
    SKIRRuntime::addLLVMOpts(rtk);

    sg->codeGenKernel(rtk);
    assert(rtk->workfn);
}

void
SKIRDPSched::callKernel(SKIRRuntimeKernel *rtk)
{
    Function *work;
    dp_splitter_work_t *state;

    if (rtk != my_kernel) {

	// pass kernels to the tbb sched
	sched = sg->getTbbSched();

	if (rtk->base_work->getName().find("skirdp") == std::string::npos) {
	    rtk->sched = sched;
	    rtk->sched->callKernel(rtk);
	    return;
	}

	// it's one of ours
	if (dp_kernels.count(rtk)) {
	    rtk->sched = sched;
	    rtk->sched->callKernel(rtk);
	    return;
	}
    
	// sorry, this seat is taken
	if (my_kernel) {
	    if (next_dp_sched)
		rtk->sched = next_dp_sched;
	    else
		next_dp_sched = rtk->sched = new SKIRDPSched(sg, width);
	    rtk->sched->callKernel(rtk);
	    return;
	}

	assert(rtk->is_fixed_rate && !rtk->is_stateful && !rtk->has_peek);

	my_kernel = rtk;

	// XXX - remove after fixing below and the inline bc
	assert((rtk->nins == 1) && (rtk->nouts == 1));
	
	extern int DPWidth;
	extern bool EnableOpenCLSched;
	extern int OpenCLMultiplier;
	width = DPWidth;

	// create split
	// 
	work = cast<Function>( getInlineCode(rtk->work->getParent(), "__SKIRRT_dp_split_work") );
	state = new dp_splitter_work_t;
	state->next_stream = 0;
	state->num_streams = width;
	state->rate = new int[width];
	int pop_rate = 0;
	for (int i=0; i<width; i++) {
	    state->rate[i] = rtk->rt_ins[0]->getPopRate();
	    if (i==0 && EnableOpenCLSched) {
		state->rate[0] *= OpenCLMultiplier;
	    }
	    pop_rate += state->rate[i];
	}

	// call the kernel instruction
	my_split = (SKIRRuntimeKernel*)sg->getRuntime().handleKernelInst(work, state);
	my_split->opt_only = true;
	my_split->is_fixed_rate = true;
	my_split->has_push = my_split->has_pop = true;

	// setup ins
	my_split->sched_ins = new SKIRRuntimeStream*[rtk->nins+1];
	for (int i=0; i<rtk->nins; i++) {
	    my_split->sched_ins[i] = rtk->rt_ins[i];
	    my_split->sched_ins[i]->setPopRate( rtk->rt_ins[0]->getPopRate()*width );
	}
	my_split->sched_ins[rtk->nins] = 0;

	// setup outs
	my_split->sched_outs = new SKIRRuntimeStream*[width+1];
	for (int i=0; i<width; i++) {
	    // TODO: for j=0; j<nins; j++ ...
	    my_split->sched_outs[i] = 
		(SKIRRuntimeStream *)sg->getRuntime().handleStreamInst(rtk->rt_ins[0]->elem_size);
	    my_split->sched_outs[i]->setPushRate( state->rate[i] );
	}
	my_split->sched_outs[width] = 0;


	// create join
	//
	work = cast<Function>( getInlineCode(rtk->work->getParent(), "__SKIRRT_dp_join_work") );
	state = new dp_splitter_work_t;
	state->next_stream = width; //look at __SKIRRT_dpq_join_work before changing this
	state->rate = new int[width];
	for (int i=0; i<width; i++)
	    state->rate[i] = rtk->rt_outs[0]->getPushRate();
	if (EnableOpenCLSched) {
	    state->rate[0] *= OpenCLMultiplier;
	}
	state->niter = 1;
	state->num_streams = width;
	state->next_stream = 0;

	// call the kernel instruction
	my_join = (SKIRRuntimeKernel*)sg->getRuntime().handleKernelInst(work, state);
	my_join->opt_only = true;
	my_join->is_fixed_rate = true;
	my_join->has_push = my_join->has_pop = true;

	// setup ins
	my_join->sched_ins = new SKIRRuntimeStream*[width+1];
	for (int i=0; i<width; i++) {
	    // TODO: for j=0; j<nouts; j++ ...
	    my_join->sched_ins[i] = 
		(SKIRRuntimeStream *)sg->getRuntime().handleStreamInst(rtk->rt_outs[0]->elem_size);
	    my_join->sched_ins[i]->setPopRate( state->rate[i] );
	    my_join->sched_ins[i]->setPeekRate(0);
	}
	my_join->sched_ins[width] = 0;

	// setup outs
	my_join->sched_outs = new SKIRRuntimeStream*[rtk->nouts+1];
	for (int i=0; i<rtk->nouts; i++) {
	    my_join->sched_outs[i] = rtk->rt_outs[i];
	    my_join->sched_outs[i]->setPushRate( state->rate[i]*width );
	}
	my_join->sched_outs[rtk->nouts] = 0;


	// create data parallel copies of rtk
	//
	for (int strm=0; strm<width; strm++) {

	    // create with kernel inst
	    SKIRRuntimeKernel *k = (SKIRRuntimeKernel*)
		sg->getRuntime().handleKernelInst(rtk->base_work, rtk->state);

	    extern bool EnableOpenCLSched;
	    if (strm == 0 && EnableOpenCLSched) {
		sg->selectScheduler(k, sg->getOpenCLSched());
		sg->getOpenCLSched()->start();
	    }

	    // setup ins
	    k->sched_ins = new SKIRRuntimeStream*[rtk->nins+1];
	    for (int i=0; i<rtk->nins; i++) {
		k->sched_ins[i] = my_split->sched_outs[strm];
		k->sched_ins[i]->setPopRate( rtk->rt_ins[i]->getPopRate() );
		k->sched_ins[i]->setPeekRate( rtk->rt_ins[i]->getPeekRate() );
	    }
	    k->sched_ins[rtk->nins] = 0;

	    // setup outs
	    k->sched_outs = new SKIRRuntimeStream*[rtk->nouts+1];
	    for (int i=0; i<rtk->nouts; i++) {
		k->sched_outs[i] = my_join->sched_ins[strm];
		k->sched_outs[i]->setPushRate( rtk->rt_outs[i]->getPushRate() );
	    }
	    k->sched_outs[rtk->nouts] = 0;

	    dp_kernels.insert(k);
	}
    }

    delete[] rtk->impl_ins;
    delete[] rtk->impl_outs;
    rtk->impl_ins = 0;
    rtk->impl_outs = 0;

    sg->getRuntime().handleCallInst(my_split, my_split->sched_ins, my_split->sched_outs);
    sg->getRuntime().handleCallInst(my_join, my_join->sched_ins, my_join->sched_outs);
    std::set<SKIRRuntimeKernel*>::iterator I = dp_kernels.begin();
    std::set<SKIRRuntimeKernel*>::iterator E = dp_kernels.end();
    for (; I!=E; ++I) {
	SKIRRuntimeKernel *k = *I;
	sg->getRuntime().handleCallInst(k, k->sched_ins, k->sched_outs);
    }

    //sg->log();
}
