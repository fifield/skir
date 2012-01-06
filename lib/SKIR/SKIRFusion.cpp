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

#include "SKIR/SKIRRuntime.h"
#include "SKIRFusion.h"
#include "SKIRUtil.h"
#include <llvm/IntrinsicInst.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Scalar.h>
#include "inline_stream_ops.h"

using namespace llvm;

// return the intersection of the output streams of K0 and the input streams of K1
void SKIRFusion::computeCommonStreams(std::set<SKIRRuntimeStream*> &streams,
				      SKIRRuntimeKernel &K0, SKIRRuntimeKernel &K1)
{
    std::set<SKIRRuntimeStream*> s;
    for (int i=0; i<K0.nouts; i++) {
	s.insert(K0.rt_outs[i]);
    }
    for (int i=0; i<K1.nins; i++) {
	SKIRRuntimeStream *in = K1.rt_ins[i];
	if (s.count(in))
	    streams.insert(in);
    }
}

//
void SKIRFusion::computeInputStreams(std::vector<SKIRRuntimeStream*> &streams,
				     std::set<SKIRRuntimeStream*> &common, 
				     SKIRRuntimeKernel &K0, SKIRRuntimeKernel &K1)
{
    for (int i=0; i<K0.nins; i++) {
	SKIRRuntimeStream *s = K0.rt_ins[i];
	if (common.count(s) == 0)
	    streams.push_back(s);
    }
    for (int i=0; i<K1.nins; i++) {
	SKIRRuntimeStream *s = K1.rt_ins[i];
	if (common.count(s) == 0)
	    streams.push_back(s);
    }
}

//
void SKIRFusion::computeOutputStreams(std::vector<SKIRRuntimeStream*> &streams,
				      std::set<SKIRRuntimeStream*> &common,
				      SKIRRuntimeKernel &K0, SKIRRuntimeKernel &K1)
{
    for (int i=0; i<K0.nouts; i++) {
	SKIRRuntimeStream *s = K0.rt_outs[i];
	if (common.count(s) == 0)
	    streams.push_back(s);
    }
    for (int i=0; i<K1.nouts; i++) {
	SKIRRuntimeStream *s = K1.rt_outs[i];
	if (common.count(s) == 0)
	    streams.push_back(s);
    }
}

// replace stream operations (intrinsics) with inline-able functions
//
void SKIRFusion::replaceStreamOps(std::set<SKIRRuntimeStream*> &common, SKIRRuntimeKernel &k)
{
    // this mostly taken from SKIRInlineStreams pass

    Value *vins;
    Value *vouts;
    std::vector<CallInst*> inline_sites;
    Function *work = k.work;
    LLVMContext &CTX = work->getContext();
    Module *mod = work->getParent();
    assert(mod && "work fn is required to have a parent module");

    Function::arg_iterator args = work->arg_begin();
    args++;	args++;
    vins = args++;
    vouts = args;

    for (inst_iterator I = inst_begin(work), E = inst_end(work); I != E; ) {
	Instruction *inst = &*I; ++I;

	if (CallInst *CI = dyn_cast<CallInst>(inst)) {
	    if (isa<SKIRIntrinsic>(inst)) {
		CallInst *newCI = 0;
		if (isa<SKIRPushInst>(inst)) {
		    Value *ops[3] = { vouts,
				      CI->getOperand(1),   /* stream */
				      CI->getOperand(2) }; /* elm */
		    static Constant *pushFCache = 0;
		    if (!pushFCache) 
			pushFCache = getInlineCode(mod, "__SKIRRT_inline_push_nocheck");
		    newCI = ReplaceCallWith("__SKIRRT_inline_push_nocheck", 
					    CI, ops, ops+3, Type::getVoidTy(CTX),
					    pushFCache);
		}
		else if (isa<SKIRPopInst>(inst)) {
		    Value *ops[3] = { vins,
				      CI->getOperand(1),   /* stream */
				      CI->getOperand(2) }; /* elmptr */
		    static Constant *popFCache;
		    if (!popFCache) 
			popFCache = getInlineCode(mod, "__SKIRRT_inline_pop_nocheck");
		    newCI = ReplaceCallWith("__SKIRRT_inline_pop_nocheck",
					    CI, ops, ops+3, Type::getVoidTy(CTX),
					    popFCache);
		}
		else if (isa<SKIRPeekInst>(inst)) {
		    static Constant *peekFCache;
		    if (!peekFCache) 
			peekFCache = getInlineCode(mod, "__SKIRRT_inline_peek_nocheck");
		    Value *ops[4] = { vins,
				      CI->getOperand(1),   /* stream */
				      CI->getOperand(2),   /* elmptr */
				      CI->getOperand(3) }; /* offset */
		    newCI = ReplaceCallWith("__SKIRRT_inline_peek_nocheck",
					    CI, ops, ops+4, Type::getVoidTy(CTX),
					    peekFCache);
		}
		else {
		    inst->dump();
		    assert(0 && "unexpected SKIR instruction");
		}
		assert(newCI);
	    }
	}
    }

}

// replace stream operations (intrinsics) with inline-able functions
//
void SKIRFusion::replaceRenumberedOps(SKIRRuntimeKernel &k,
				      std::vector<Instruction*> &bufs,
				      std::vector<Instruction*> &idxs)
{
    Function *work = k.work;
    LLVMContext &CTX = work->getContext();
    Module *mod = work->getParent();
    std::vector<CallInst*> inline_sites;
    assert(mod && "work fn is required to have a parent module");

    for (inst_iterator I = inst_begin(work), E = inst_end(work); I != E; ) {
	Instruction *inst = &*I; ++I;

	if (CallInst *CI = dyn_cast<CallInst>(inst)) {
	    CallInst *newCI = 0;
	    if (isa<SKIRPushInst>(inst)) {
		ConstantInt *stream_idx = dyn_cast<ConstantInt>(inst->getOperand(1));
		int idx = (stream_idx->getZExtValue() - 1000);
		if (idx<0) continue;
		Value *ops[3] = { bufs[idx],
				  idxs[idx*2],
				  CI->getOperand(2) }; /* elm */
		static Constant *pushFCache = 0;
		if (!pushFCache) 
		    pushFCache = getInlineCode(mod, "__SKIRRT_inline_push_fuse_4");
		newCI = ReplaceCallWith("__SKIRRT_inline_push_fuse_4",
					CI, ops, ops+3, Type::getVoidTy(CTX),
					pushFCache);
	    }
	    else if (isa<SKIRPopInst>(inst)) {
		ConstantInt *stream_idx = dyn_cast<ConstantInt>(inst->getOperand(1));
		int idx = (stream_idx->getZExtValue() - 1000);
		if (idx<0) continue;
		Value *ops[3] = { bufs[idx],
				  idxs[(idx*2)+1],
				  CI->getOperand(2) }; /* elmptr */
		static Constant *popFCache = 0;
		if (!popFCache) 
		    popFCache = getInlineCode(mod, "__SKIRRT_inline_pop_fuse_4");
		newCI = ReplaceCallWith("__SKIRRT_inline_pop_fuse_4",
					CI, ops, ops+3, Type::getVoidTy(CTX),
					popFCache);
	    }
	    else if (isa<SKIRPeekInst>(inst)) {
		assert(0 && "peek");
	    }
	    if (newCI)
		inline_sites.push_back(newCI);
	}
    }

    // do inlines
    std::vector<CallInst*>::iterator iter;
    for (iter = inline_sites.begin(); iter != inline_sites.end(); ) {
	CallInst *I = *iter; ++iter;
	assert( InlineFunction(I) );
    }

}

// replace stream operations (intrinsics) with inline-able functions
//
void SKIRFusion::renumberStreamOps(SKIRRuntimeKernel &newK,
				   SKIRRuntimeKernel &rtk)
{
    // this mostly taken from SKIRInlineStreams pass

    Value *vins;
    Value *vouts;
    std::vector<CallInst*> inline_sites;
    Function *work = rtk.work;
    //LLVMContext &CTX = work->getContext();
    Module *mod = work->getParent();
    assert(mod && "work fn is required to have a parent module");

    Function::arg_iterator args = work->arg_begin();
    args++;	args++;
    vins = args++;
    vouts = args;

    for (inst_iterator I = inst_begin(work), E = inst_end(work); I != E; ) {
	Instruction *inst = &*I; ++I;

	if (isa<SKIRPushInst>(inst) || isa<SKIRPopInst>(inst) || isa<SKIRPeekInst>(inst)) {

	    ConstantInt *stream_idx = dyn_cast<ConstantInt>(inst->getOperand(1));
	    int idx = stream_idx->getZExtValue();

	    SKIRRuntimeStream *rs;
	    if (isa<SKIRPushInst>(inst))
		rs = rtk.rt_outs[idx];
	    else
		rs = rtk.rt_ins[idx];
		    
	    int new_idx = -1;
	    for (int i=0; new_idx<0 && i<newK.nins; i++)
		if (rs == newK.rt_ins[i]) 
		    new_idx = i;
	    for (int i=0; new_idx<0 && i<newK.nouts; i++)
		if (rs == newK.rt_outs[i]) 
		    new_idx = i;
	    for (int i=0; new_idx<0 && i<newK.nints; i++)
		if (rs == newK.rt_ints[i]) 
		    new_idx = i + 1000;
	    assert(new_idx >= 0);

	    inst->setOperand(1, ConstantInt::get(inst->getOperand(1)->getType(), new_idx));
	}
    }
}

static inline int myLCM(int x, int y)
{
    assert(x && y);
    int i = y;
    while (i%x) i+=y;
    return i;
}

// return a newly generated kernel representing the fusion of K0 and K1
//
SKIRRuntimeKernel *SKIRFusion::runOnKernels(SKIRRuntimeKernel &K0, SKIRRuntimeKernel &K1)
{
    SKIRRuntimeKernel *newKernel = 0;
    
    std::vector<SKIRRuntimeStream*> input_streams;
    std::vector<SKIRRuntimeStream*> output_streams;
    std::set<SKIRRuntimeStream*> common_streams;

    if (K0.has_peek || K1.has_peek) return 0;
    if (!K0.is_fixed_rate || !K1.is_fixed_rate) return 0;

    //
    computeCommonStreams(common_streams, K0, K1);
    computeInputStreams(input_streams, common_streams, K0, K1);
    computeOutputStreams(output_streams, common_streams, K0, K1);

    int i;
    int niter0, niter1, niter_lcm;
    niter0 = niter1 = niter_lcm = 1;
    {
	std::set<SKIRRuntimeStream*>::iterator I,E;
	for (i=0, I=common_streams.begin(),E=common_streams.end(); I!=E; ++I, ++i) {
	    SKIRRuntimeStream *s = *I;
	    // match the rates of the common streams
	    // nb: if peek was supported this would change
	    niter_lcm = myLCM(niter_lcm, myLCM(s->si->push_rate, s->si->pop_rate));
	    int n0 = niter_lcm / s->si->push_rate;
	    int n1 = niter_lcm / s->si->pop_rate;
	    niter0 = n0;
	    niter1 = n1;
	}
    }
    // check rates
    for (i=0; i<K0.nins; i++) {
	SKIRRuntimeStream *s = K0.rt_ins[i];
	if ((s->si->pop_rate * s->elem_size * niter0) > STREAM_BUFFER_SIZE)
	    return 0;
    }
    for (i=0; i<K0.nouts; i++) {
	SKIRRuntimeStream *s = K0.rt_outs[i];
	if ((s->si->push_rate * s->elem_size * niter0) > STREAM_BUFFER_SIZE)
	    return 0;
    }
    for (i=0; i<K1.nins; i++) {
	SKIRRuntimeStream *s = K1.rt_ins[i];
	if ((s->si->pop_rate * s->elem_size * niter1) > STREAM_BUFFER_SIZE)
	    return 0;
    }
    for (i=0; i<K1.nouts; i++) {
	SKIRRuntimeStream *s = K1.rt_outs[i];
	if ((s->si->push_rate * s->elem_size * niter1) > STREAM_BUFFER_SIZE)
	    return 0;
    }

    // new kernel
    //
    newKernel = new SKIRRuntimeKernel(sg.getRuntime().nextKernelID());
    newKernel->nins = input_streams.size();
    newKernel->nouts = output_streams.size();
    newKernel->nints = common_streams.size();

    // new kernel streams
    //
    newKernel->rt_ins = new SKIRRuntimeStream*[newKernel->nins+1];
    newKernel->rt_outs = new SKIRRuntimeStream*[newKernel->nouts+1];
    newKernel->rt_ints = new SKIRRuntimeStream*[newKernel->nints+1];

    newKernel->impl_ins = (void**)new skir_stream_t*[newKernel->nins+1];
    newKernel->impl_outs = (void**)new skir_stream_t*[newKernel->nouts+1];

    // these arrays must be null terminated
    newKernel->rt_ins[newKernel->nins] = 0;
    newKernel->rt_outs[newKernel->nouts] = 0;
    newKernel->rt_ints[newKernel->nints] = 0;
    newKernel->impl_ins[newKernel->nins] = 0;
    newKernel->impl_outs[newKernel->nouts] = 0;

    int nints = 0;
    {
	std::set<SKIRRuntimeStream*>::iterator I,E;
	for (I=common_streams.begin(),E=common_streams.end(); I!=E; ++I) {
	    SKIRRuntimeStream *s = *I;
	    s->si->src = newKernel;
	    s->si->dst = newKernel;
	    newKernel->rt_ints[nints++] = s;
	}
	assert(nints == newKernel->nints);
    }
    {
	// setup newKernel's input/output streams
	std::vector<SKIRRuntimeStream*>::iterator I,E;
	for (i=0,I=input_streams.begin(),E=input_streams.end(); I!=E; ++I, ++i) {
	    SKIRRuntimeStream *s = *I;
	    if (s->type != SKIRRuntimeStream::SHARED)
		s->si->dst = newKernel;
	    else
		s->si->dst = (void*)2;
	    newKernel->rt_ins[i] = s;
	    newKernel->impl_ins[i] = newKernel->rt_ins[i]->si;
	}
	for (i=0,I=output_streams.begin(),E=output_streams.end(); I!=E; ++I, ++i) {
	    SKIRRuntimeStream *s = *I;
	    if (s->type != SKIRRuntimeStream::SHARED)
		s->si->src = newKernel;
	    else
		s->si->src = (void*)2;
	    newKernel->rt_outs[i] = s;
	    newKernel->impl_outs[i] = newKernel->rt_outs[i]->si;
	}
    }

    // adjust rates
    for (int i=0; i<K0.nins; i++) {
	SKIRRuntimeStream *s = K0.rt_ins[i];
	s->si->pop_rate *= niter0;
	assert((s->si->pop_rate * K0.rt_ins[i]->elem_size) <= STREAM_BUFFER_SIZE);
    }
    for (int i=0; i<K0.nouts; i++) {
	SKIRRuntimeStream *s = K0.rt_outs[i];
	s->si->push_rate *= niter0;
	assert((s->si->push_rate * K0.rt_outs[i]->elem_size) <= STREAM_BUFFER_SIZE);
    }
    for (int i=0; i<K1.nins; i++) {
	SKIRRuntimeStream *s = K1.rt_ins[i];
	s->si->pop_rate *= niter1;
	assert((s->si->pop_rate * K1.rt_ins[i]->elem_size) <= STREAM_BUFFER_SIZE);
    }
    for (int i=0; i<K1.nouts; i++) {
	SKIRRuntimeStream *s = K1.rt_outs[i];
	s->si->push_rate *= niter1;
	assert((s->si->push_rate * K1.rt_outs[i]->elem_size) <= STREAM_BUFFER_SIZE);
    }

    // new kernel work function
    //
    if (K0.is_const_idx && K1.is_const_idx) {
	renumberStreamOps(*newKernel, K0);
	renumberStreamOps(*newKernel, K1);

	newKernel->nints = common_streams.size();
	newKernel->rt_ints[newKernel->nints] = 0;
    } else {
	assert(0 && "renumbering requires is_const_idx == true");
#if 0
	// add internal streams to new kernel
	for (int i=0; i<K0.nints; i++) {
	    SKIRRuntimeStream *s = K0.rt_ints[i];
	    s->si->src = newKernel;
	    s->si->dst = newKernel;
	    s->si->push_rate *= niter0;
	    s->si->pop_rate *= niter0;
	    newKernel->rt_ints[nints++] = s;
	}
	for (int i=0; i<K1.nints; i++) {
	    SKIRRuntimeStream *s = K1.rt_ints[i];
	    s->si->src = newKernel;
	    s->si->dst = newKernel;
	    s->si->push_rate *= niter1;
	    s->si->pop_rate *= niter1;
	    newKernel->rt_ints[nints++] = s;
	}
	assert(nints == newKernel->nints);

	replaceStreamOps(common_streams, K0);
	replaceStreamOps(common_streams, K1);
#endif
    }

    Function *tmpF;
    if (niter0 == 1 && niter1 == 1)
	tmpF = makeWorkFromSkel(&K0, "__SKIRRT_workfn_inline2_1_1");
    else
	tmpF = makeWorkFromSkel(&K0, "__SKIRRT_workfn_inline2");
    Module *mod = K1.work->getParent();
    mod->getFunctionList().push_back(tmpF);

    newKernel->base_work = newKernel->work = makeWorkFromSkel(&K1, tmpF->getName().str().c_str());
    newKernel->work->setName(K0.work->getName() + K1.work->getName());
    mod->getFunctionList().push_back(newKernel->work);

    //newKernel->opt_only = true;
    newKernel->is_hier = false;
    newKernel->is_stateful = K0.is_stateful || K1.is_stateful;
    newKernel->is_fixed_rate = K0.is_fixed_rate && K1.is_fixed_rate;
    newKernel->is_const_idx = K0.is_const_idx && K1.is_const_idx;
    newKernel->has_push = (newKernel->nouts > 0);
    newKernel->has_pop = (newKernel->nouts > 0);
    newKernel->has_peek = K0.has_peek || K1.has_peek;

    assert(!newKernel->has_peek && newKernel->is_fixed_rate);// && !newKernel->is_stateful);

    if (newKernel->is_const_idx) {
	LLVMContext &CTX = newKernel->work->getContext();
	std::vector<Instruction *> bufs;
	std::vector<Instruction *> idxs;

	BasicBlock::iterator I = newKernel->work->getEntryBlock().begin();
	while (isa<AllocaInst>(I)) ++I;
	Instruction *insert = &*I;

	for (int i=0; i<newKernel->nints; i++) {
	    int size = newKernel->rt_ints[i]->elem_size * newKernel->rt_ints[i]->si->push_rate;
	    int size2 = newKernel->rt_ints[i]->elem_size * newKernel->rt_ints[i]->si->pop_rate;
	    assert (size == size2);

	    bufs.push_back( new AllocaInst(Type::getInt8Ty(CTX),
					   ConstantInt::get(Type::getInt32Ty(CTX), size),
					   "strm_buf", insert) );
	    Instruction *idx = new AllocaInst(Type::getInt32Ty(CTX), "strm_wr_idx", insert);
	    new StoreInst(ConstantInt::get(Type::getInt32Ty(CTX), 0), idx, insert);
	    idxs.push_back(idx);
	    idx = new AllocaInst(Type::getInt32Ty(CTX), "strm_rd_idx", insert);
	    new StoreInst(ConstantInt::get(Type::getInt32Ty(CTX), 0), idx, insert);
	    idxs.push_back(idx);
	}

	replaceRenumberedOps(*newKernel, bufs, idxs);
    }

    //
    inline_inline_t *state = new inline_inline_t;

    state->rt_state0 = K0.rt_state;
    state->state0 = K0.state;
    state->ins0 = (skir_stream_t**)K0.impl_ins;
    state->outs0 = (skir_stream_t**)K0.impl_outs;
    state->niter0 = niter0;
    state->iter0 = 0;

    state->rt_state1 = K1.rt_state;
    state->state1 = K1.state;
    state->ins1 = (skir_stream_t**)K1.impl_ins;
    state->outs1 = (skir_stream_t**)K1.impl_outs;
    state->niter1 = niter1;
    state->iter1 = 0;

    //adjustRates(state->ins0, state->outs0, state->niter0);
    //adjustRates(state->ins1, state->outs1, state->niter1);

    newKernel->state = state;

    //
    //
    assert(K0.sched == K1.sched);
    newKernel->sched = K0.sched;
    assert(K0.cg == K1.cg);
    newKernel->cg = K0.cg;

    return newKernel;
}
