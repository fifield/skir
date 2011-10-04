#ifdef USE_OPENCL

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/LLVMContext.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/InstIterator.h>
#include "llvm/Assembly/Parser.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/ADT/SmallSet.h"
#include <llvm/InstrTypes.h>

#include <SKIR/SKIRStream.h>
#include <SKIR/SKIRRuntime.h>
#include "SKIRTbbSched.h"
#include "SKIRRuntimeGraph.h"
#include "SKIRRuntimeKernel.h"
#include "SKIRUtil.h"
#include "SKIRTiming.h"

#include <SKIROpenCLSched.h>
#include <SKIRSingleThreadSched.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>
#include <assert.h>

#include <utility>
#define __NO_STD_VECTOR // Use cl::vector and cl::string and 
#define __NO_STD_STRING  // not STL versions, more on this later
#include <CL/cl.hpp>

using namespace llvm;

// enable/disable profiling
#define SKIRCL_QUEUE_PROFILING_ENABLE CL_QUEUE_PROFILING_ENABLE
//#define SKIRCL_QUEUE_PROFILING_ENABLE 1

static inline void
checkErr(cl_int err, const char * name)
{
    if (err != CL_SUCCESS) {
	std::cerr << "ERROR: " << name
	       << " (" << err << ")" << std::endl;
        //assert(0);
    }
}

static cl::vector<cl::Device> devices;
static cl::Context *context;
static cl::CommandQueue *queue;

//
// helper kernel and stream code
//
extern "C"
{

#include "inline_stream_ops.h"

static const unsigned OPENCL_BUFFER_SIZE = 2*1024*1024;

typedef struct {
    SKIRRuntimeKernel *rtk;
    void *orig_state;
    size_t orig_state_size;
    work_function *orig_workfn;

    cl::Kernel *cl_kernel;
    cl::Buffer *insB;
    cl::Buffer *devinsB;
    cl::Buffer *outsB;
    cl::Buffer *devoutsB;
    cl::Buffer *stateB;
    cl::Buffer *returnB;
    int kernel_return;

    size_t niter;
    
    size_t in_bytes_per_iter;
    size_t out_bytes_per_iter;

    size_t total_ns;
    size_t total_niter;

    size_t in_rd_idx;
    size_t in_wr_idx;
    size_t out_rd_idx;
    size_t out_wr_idx;

    char *in_buf;
    char *out_buf;

} opencl_state_t;

static opencl_state_t *
new_opencl_state_t(SKIRRuntimeKernel *rtk)
{
    opencl_state_t *new_state = (opencl_state_t *)calloc(1,sizeof(opencl_state_t));
    new_state->rtk = rtk;
    new_state->orig_state = rtk->state;
    new_state->orig_workfn = rtk->workfn;
    
    // compute amount of data read/written per iteration
    for (int j=0; j<rtk->nins; j++) {
	skir_stream_t *s = (skir_stream_t *)rtk->impl_ins[j];
	new_state->in_bytes_per_iter += s->pop_rate * s->elem_size;
    }
    for (int j=0; j<rtk->nouts; j++) {
	skir_stream_t *s = (skir_stream_t *)rtk->impl_outs[j];
	new_state->out_bytes_per_iter += s->push_rate * s->elem_size;
    }

    if (0/*verbose*/) {
	errs() << rtk->work->getName() << ": ";
	errs() << "\n\t inb/iter: " << new_state->in_bytes_per_iter;
	errs() << "\n\toutb/iter: " << new_state->out_bytes_per_iter;
	errs() << "\n";
    }
    return new_state;
}

static inline size_t
__SKIRRT_opencl_pop_inputs(opencl_state_t *s, skir_stream_t *ins[])
{
    SKIRRuntimeKernel *k = s->rtk;

    // number of iterations of space in the input buffer
    size_t niter = (OPENCL_BUFFER_SIZE - s->in_wr_idx) / s->in_bytes_per_iter;
    void *v;

    // number of iterations of data in the input stream(s)
    niter = std::min(niter, __SKIRRT_inline_compute_niters(&v, ins, k->nins, 0, 0));

    for (size_t i=0; i<niter; i++) {
	for (int j=0; j<k->nins; j++) {
	    skir_stream_t *ss = (skir_stream_t*)ins[j];
	    for (int l=0; l<ss->pop_rate; l++) {
		__SKIRRT_inline_pop_nocheck(ins, j, &s->in_buf[s->in_wr_idx]);
		s->in_wr_idx += ss->elem_size;
	    }
	}
    }

    return niter;
}

static inline size_t
__SKIRRT_opencl_push_outputs(opencl_state_t *s, skir_stream_t *outs[])
{
    SKIRRuntimeKernel *k = s->rtk;
    size_t niter = 0;
    void *v;

    // service any data in the output buffer
    if (s->out_rd_idx != s->out_wr_idx) {
	
	// number of iterations of data in the output buffer
	niter = (s->out_wr_idx - s->out_rd_idx) / s->out_bytes_per_iter;
	
	// number of iterations of space in the output stream(s)
	niter = std::min(niter, __SKIRRT_inline_compute_niters(&v, 0, 0, outs, k->nouts));
	
	for (size_t i=0; i<niter; i++) {
	    for (int j=0; j<k->nouts; j++) {
		skir_stream_t *ss = outs[j];
		for (int l=0; l<ss->push_rate; l++) {
		    __SKIRRT_inline_push_nocheck(outs, j, &s->out_buf[s->out_rd_idx]);
		    s->out_rd_idx += ss->elem_size;
		}
	    }
	}
    }

    return niter;
}

// already defined in inline_stream_ops.h in a way that won't work here
#undef START_TSC
#undef GET_TSC
#define START_TSC(tsc)  (tsc = __rdtscll())
#define GET_TSC(tsc)  (tsc = (__rdtscll() - tsc))

static void *
__SKIRRT_opencl_workfn(skir_rt_state_t   *rt_state, 
		       opencl_state_t *s,
		       skir_stream_t *ins[],
		       skir_stream_t *outs[])
{
    SKIRRuntimeKernel *k = s->rtk;

    // do the easy thing for now; on every iteration:
    // 1) push any data we can out of the output buffer
    // 2) pop as much data as possible into the input buffer
    // 3) do work

    //int thresh = 1;

    // start timing
    START_TSC(rt_state->cycles);

    while (1) {

	__SKIRRT_opencl_pop_inputs(s, ins);
	size_t input_niter = (s->in_wr_idx - s->in_rd_idx) / s->in_bytes_per_iter;

	__SKIRRT_opencl_push_outputs(s, outs);
	if (s->out_rd_idx == s->out_wr_idx) {
	    s->out_rd_idx = s->out_wr_idx = 0;
	}
	size_t output_niter = (OPENCL_BUFFER_SIZE - s->out_wr_idx) / s->out_bytes_per_iter;

	// niter = min( data waiting in input buffer, free space available in output buffer )
	size_t niter = std::min(input_niter, output_niter);

	// if the number of iterations is over the threshold, do work
	//if (niter >= (OPENCL_BUFFER_SIZE/ std::max(s->in_bytes_per_iter,s->out_bytes_per_iter))/2)
	if (niter > 0)
	{
	    //thresh = (OPENCL_BUFFER_SIZE/ std::max(s->in_bytes_per_iter,s->out_bytes_per_iter))/2;
	    cl_int err;

	    checkErr(s->cl_kernel->setArg(0, *s->returnB), "Kernel::setArg()");
	    checkErr(s->cl_kernel->setArg(1, *s->stateB), "Kernel::setArg()");
	    checkErr(s->cl_kernel->setArg(2, *s->devinsB), "Kernel::setArg()");
	    checkErr(s->cl_kernel->setArg(3, *s->devoutsB), "Kernel::setArg()");

	    s->niter = niter;
	    cl::vector<cl::Event> events0;
	    cl::vector<cl::Event> events1;
	    cl::Event wr_event, st8_event, kernel_event, rd_event;

	    err = queue->enqueueWriteBuffer(*s->devinsB,
					    CL_FALSE,
					    0,
					    niter*s->in_bytes_per_iter,
					    s->in_buf+s->in_rd_idx,
					    0,
					    &wr_event);
	    checkErr(err, "ComamndQueue::enqueueWriteBuffer()");
	    events0.push_back(wr_event);
	    events1.push_back(wr_event);

	    err = queue->enqueueWriteBuffer(*s->stateB,
					    CL_FALSE,
					    0,
					    s->orig_state_size,
					    s->orig_state,
					    0,
					    &st8_event);
	    checkErr(err, "ComamndQueue::enqueueWriteBuffer()");
	    events0.push_back(st8_event);
	    events1.push_back(st8_event);


	    err = queue->enqueueNDRangeKernel(*s->cl_kernel, 
					      cl::NullRange,
					      cl::NDRange(niter),
					      cl::NullRange,
					      &events0,
					      &kernel_event);
	    checkErr(err, "CommandQueue::enqueueNDRangeKernel()");
	    events1.push_back(kernel_event);
	    err = queue->enqueueReadBuffer(*s->devoutsB,
					   CL_FALSE,
					   0,
					   niter*s->out_bytes_per_iter,
					   s->out_buf+s->out_wr_idx,
					   &events1,
					   &rd_event);
	    checkErr(err, "ComamndQueue::enqueueReadBuffer()");

	    rd_event.wait();

#if SKIRCL_QUEUE_PROFILING_ENABLE
	    cl_ulong start_time, stop_time;
	    err = wr_event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &start_time);
	    checkErr(err, "getProfilingInfo");

	    err = rd_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &stop_time);
	    checkErr(err, "getProfilingInfo");
	    
	    //errs() << "time:          " << stop_time - start_time << "\n";
	    //errs() << "niter:         " << niter << "\n";
	    //errs() << "time per iter: " << ((double)(stop_time - start_time))/((double)niter) << "\n";
	    s->total_ns = stop_time - start_time;
	    s->total_niter = niter;
#endif
	    rt_state->niter += niter;

	    s->in_rd_idx += (niter*s->in_bytes_per_iter);
	    s->out_wr_idx += (niter*s->out_bytes_per_iter);

	    if (s->in_rd_idx == s->in_wr_idx)
		s->in_rd_idx = s->in_wr_idx = 0;

	    // XXX ignore kernel return

	} else {
	    //thresh = thresh >> 1;
	    //if (thresh) continue;
	    // couldn't execute, bail out
	    void *v = 0;
	    if ((s->out_rd_idx == s->out_wr_idx) && !input_niter) { //output and input empty
		__SKIRRT_inline_compute_niters(&v, ins, k->nins, outs, k->nouts);
	    } if (!(s->out_rd_idx == s->out_wr_idx)) { // output not empty, favor outputs
		__SKIRRT_inline_compute_niters(&v, 0, 0, outs, k->nouts);
	    } else {
		__SKIRRT_inline_compute_niters(&v, ins, k->nins, 0, 0);
	    }
	    if (v == (void*)1) {
		if (0/*verbose*/) {
		    errs() << "time:          " << s->total_ns << "\n";
		    errs() << "niter:         " << s->total_niter << "\n";
		    errs() << "time per iter: "<< (double)s->total_ns/(double)s->total_niter<<"\n";
		}
	    }

	    // stop timing
	    GET_TSC(rt_state->cycles);

	    return v;
	}

    } // while(1)

    // unreachable
    assert(0);
}

} // extern C

namespace llvm {

static ManagedStatic<LLVMContext> OpenCLContext;
static LLVMContext& getCLContext() 
{
    return *OpenCLContext;
}

SKIROpenCLSched::SKIROpenCLSched(SKIRRuntimeGraph *stream_graph) 
  : sg(stream_graph)
{
    the_single_sched = sg->getTbbSched();//new SKIRSingleThreadSched(sg);
    verbose = false;
    running = 0;
    opencl_module = 0;
}

SKIROpenCLSched::~SKIROpenCLSched()
{
    delete the_single_sched;
}

Module *
SKIROpenCLSched::getCLModule()
{
    if (!opencl_module) {
	// load the runtime bitcode library
	std::string skir_path(getenv("SKIR_ROOT"));
	std::string errormsg;
	skir_path = skir_path + "/skir/build/lib/skir_opencl_mod.bc";
	if (MemoryBuffer *buffer = MemoryBuffer::getFileOrSTDIN(skir_path, &errormsg)) {
	    setCLModule(ParseBitcodeFile(buffer, getCLContext(), &errormsg));
	    delete buffer;
	}
	if (!opencl_module) {
	    errs() << "bitcode didn't read correctly:";
	    if (errormsg.size()) errs() << errormsg;
	    errs() << "\n";
	    assert(0);
	    //return NULL;
	}
    }
    return opencl_module;
}

const Type *
SKIROpenCLSched::getStateType(Value *statePtr)
{
    const PointerType *statePtrTy = cast<PointerType>(statePtr->getType());
    const Type *stateTy = statePtrTy->getElementType();
    const Type *ty = PointerType::get(stateTy, 1);

    // hack.  This forces the vtable pointer in a c++ class to be 8 bytes in the
    // opencl world (instead of a 4 byte ptr) so that all the state that follows
    // is aligned correctly.
    unsigned int zero = 0;
    if (const StructType *sTy = dyn_cast<StructType>(stateTy)) {
	if ( const StructType *sTy2 = dyn_cast<StructType>(sTy->getTypeAtIndex(zero)) ) {
	    if ( sTy2->getTypeAtIndex(zero) == Type::getInt8PtrTy(sTy2->getContext(),0) ){
		std::vector<const Type*> types;
		types.push_back( ArrayType::get(Type::getInt8Ty(getCLContext()), 8) );
		for (unsigned int i=1; i<sTy->getNumElements(); i++) {
		    const Type *tyIdx = sTy->getTypeAtIndex(i);
		    types.push_back(tyIdx);
		}
		const Type *newTy = PointerType::get( StructType::get(getCLContext(), types), 1 );
		//getCLModule()->addTypeName( getCLModule()->getTypeName(stateTy)+".skircl", newTy);
		return newTy;
	    }
	}
    }
    return ty;
}

void
SKIROpenCLSched::runCodeGen(SKIRRuntimeKernel *rtk)
{
    if (!rtk->fixed_sched && rtk->base_work->getName().find("skircl") == std::string::npos) {
	// pass to default scheduler for codegen
	sg->getRuntime().getSched()->runCodeGen(rtk);
    } 
    else {
	//
	// run opencl codegen
	//
	MutexGuard locked(rtk->cg->lock);

	if (rtk->workfn == (work_function*)__SKIRRT_opencl_workfn)
	    return;

	// reset work function
	rtk->work = rtk->base_work;

	// make opencl copy of work function and adjust prototype

	const Type *retTy = Type::getInt32PtrTy(getCLContext(), 1);
	const Type *charStarTy = Type::getInt8PtrTy(getCLContext(),1);
	const Type *oldStateTy = ((Value*)rtk->work->arg_begin())->getType();
	const Type *newStateTy = getStateType(rtk->work->arg_begin());

	std::vector<const Type*> arg_types;
	arg_types.push_back(retTy);
	arg_types.push_back(newStateTy);
	//arg_types.push_back(charStarTy);
	arg_types.push_back(charStarTy);
	arg_types.push_back(charStarTy);
	FunctionType *new_work_type = FunctionType::get(Type::getVoidTy(getCLContext()),
							arg_types, false);

	// create new function with new type
	Function *new_work = Function::Create(new_work_type, 
					      rtk->work->getLinkage(),
					      rtk->base_work->getName()+"_opencl",
					      getCLModule());

	DenseMap<const Value*, Value*> value_map;
	Function::arg_iterator new_work_arg = new_work->arg_begin();
	new_work_arg->setName("ret_value");

	new_work_arg++;

	// set the names of the arguments for the new function and
	// setup the value_map
	Function::const_arg_iterator old_work_arg = rtk->work->arg_begin();
	new_work_arg->setName(old_work_arg->getName());
	value_map[old_work_arg++] = new_work_arg++;
	new_work_arg->setName(old_work_arg->getName());
	value_map[old_work_arg++] = new_work_arg++;
	new_work_arg->setName(old_work_arg->getName());
	value_map[old_work_arg++] = new_work_arg++;

	SmallVector<ReturnInst*, 8> ret;
	CloneFunctionInto(new_work, rtk->work, value_map, ret, "", 0);

	new_work_arg = new_work->arg_begin();
	new_work_arg++;
	Value *state = new_work_arg;
	const PointerType *stateTy = cast<PointerType>(state->getType());

	SmallSet<Value*, 8> use_set;
	for (Value::use_iterator ui = state->use_begin(), ue = state->use_end(); ui != ue; ++ui)
	    use_set.insert(*ui);

	// fixup the address spaces
	SmallSet<Value*, 8> visited_set;
	while (use_set.size()) {

	    Value *v = *use_set.begin();
	    use_set.erase(v);
	    if (visited_set.count(v)) continue;
	    visited_set.insert(v);

	    if (Instruction *inst = dyn_cast<Instruction>(v)) {

		// if the type of the instruction is a pointer with an address space
		// different from the state address space
		if (const PointerType *pty = dyn_cast<PointerType>(inst->getType())) {
		    if (pty->getAddressSpace() != stateTy->getAddressSpace()) {

			Instruction *new_inst = NULL;

			const Type *newTy;
			if (pty == oldStateTy)
			    newTy = newStateTy;
			else
			    newTy = PointerType::get(pty->getElementType(),
						     stateTy->getAddressSpace());

			if (isa<BitCastInst>(inst)) {
			    new_inst = new BitCastInst(inst->getOperand(0), newTy, "", inst);
			}
			else if (isa<IntToPtrInst>(inst)) {
			    new_inst = new IntToPtrInst(inst->getOperand(0), newTy, "", inst);
			}
			else if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(inst)) {
			    unsigned nidx = gep->getNumIndices();
			    Value *idxs[nidx];
			    User::op_iterator iter = gep->idx_begin();
			    for (unsigned i=0; i<nidx; i++, ++iter)
				idxs[i] = iter->get();
			    Value *Ptr =gep->getPointerOperand();
			    assert( GetElementPtrInst::getIndexedType(Ptr->getType(), 
								      idxs, idxs+nidx) );
			    new_inst = GetElementPtrInst::Create(gep->getPointerOperand(), 
								 idxs, idxs+nidx, "", inst);
			}
			else if(PHINode *phi = dyn_cast<PHINode>(inst)) {
			    PHINode *new_phi = PHINode::Create(newTy, inst->getName(), inst);
			    unsigned i = 0, n = phi->getNumIncomingValues();
			    for (i=0; i<n; i++) {
				Value *iv = phi->getIncomingValue(i);
				BasicBlock *ib = phi->getIncomingBlock(i);
				if ( (cast<PointerType>(iv->getType()))->getAddressSpace() != 
				     stateTy->getAddressSpace() ) {
				    use_set.insert(iv);
				}
				new_phi->addIncomingUnchecked(iv, ib);
			    }
			    new_inst = new_phi;
			}
			else {
			    assert(0 && "Unhandled instruction type");
			}
			if (new_inst) {
			    inst->uncheckedReplaceAllUsesWith(new_inst);
			    inst->eraseFromParent();
			    Value::use_iterator ui = new_inst->use_begin();
			    Value::use_iterator ue = new_inst->use_end(); 
			    while (ui != ue) use_set.insert(*ui++);
			}		      
		    }
		} else {
		    //if (PtrToIntInst *p2i = dyn_cast<PtrToIntInst>(inst)) {
		    //for (Value::use_iterator I=p2i->use_begin(), E=p2i->use_end(); I!=E; ++I)
		    //use_set.insert(*I);
		    //}
		    //else {
		    for (Value::use_iterator I=inst->use_begin(), E=inst->use_end(); I!=E; ++I)
			use_set.insert(*I);
		    //}
		}
	    }
	}

	//replaceUsesOfType(new_work, oldStateTy, newStateTy);
	

	// a nasty way of making sure the context is changed:
	// write the module out as a string...
	std::string base_work_asm;
	raw_string_ostream o(base_work_asm);
	o << *getCLModule();
	// ...then read it back...
	const char *c = base_work_asm.c_str();
	MemoryBuffer *b = MemoryBuffer::getMemBuffer(c, c+base_work_asm.length());
	assert(b && "Couldn't create memory buffer!");
	SMDiagnostic Err;
	Module *m = ParseAssembly(b, 0, Err, getCLContext());
	// ...and replace the old module.
	if (m) {
	    setCLModule(m);
	} else {
	    Err.Print("includeAssembly", errs());
	    assert(0);
	}

	// set the work function
	rtk->work = getCLModule()->getFunction(rtk->base_work->getName().str()+"_opencl");
	assert(rtk->work && "Can't find newly copied function");

	// now add the opencl bits

	Function::arg_iterator work_arg_iter = rtk->work->arg_begin();
	Value *arg_return = work_arg_iter++;
	/*Value *arg_state = */ work_arg_iter++;
	Value *arg_ins = work_arg_iter++;
	Value *arg_outs = work_arg_iter;

	// replace return instructions with stores followed by return void
	for (inst_iterator I = inst_begin(rtk->work), E = inst_end(rtk->work); I != E; ) {
	    Instruction *inst = &*I;
	    ++I;
	    assert(&inst->getContext() == &getCLContext());
	    if (ReturnInst *RI = dyn_cast<ReturnInst>(inst)) {
		new StoreInst(RI->getReturnValue(), arg_return, RI);
		ReturnInst::Create(getCLContext(), 0, RI);
		RI->eraseFromParent();
	    }
	}

	opencl_state_t *new_state = new_opencl_state_t(rtk);
	TargetData TD(getCLModule());
	const StructLayout *structLayout 
	    = TD.getStructLayout(cast<StructType>(stateTy->getElementType()));
	new_state->orig_state_size = structLayout->getSizeInBytes();
	    //= TD.getTypeAllocaSize(rtk->work->arg_begin());

	if (verbose) std::cout << "state size " << new_state->orig_state_size << "\n";
	       

	// find a good insertion point for prolog stuff
	BasicBlock::iterator BBI = rtk->work->getEntryBlock().begin();
	while (isa<PHINode>(BBI) || isa<AllocaInst>(BBI)) ++BBI;
	Instruction *insert = &*BBI;
	
	// get the opencl kernel global id
	Instruction *global_id 
	    = CallInst::Create(getCLModule()->getFunction("get_global_id"),
			       ConstantInt::get(Type::getInt32Ty(getCLContext()),0),
			       "global_id", insert);

	// generate offsets into the input/output buffer for every input/outupt stream
	//
	Value *in_global_offset = 0;
	if (rtk->nins) {
	    in_global_offset = ConstantInt::get(Type::getInt32Ty(getCLContext()),
						new_state->in_bytes_per_iter);
	    in_global_offset = BinaryOperator::Create(BinaryOperator::Mul, in_global_offset,
						      global_id, "", insert);
	}
	Value *out_global_offset = 0;
	if (rtk->nouts) {
	    out_global_offset = ConstantInt::get(Type::getInt32Ty(getCLContext()),
						 new_state->out_bytes_per_iter);
	    out_global_offset = BinaryOperator::Create(BinaryOperator::Mul, out_global_offset,
						       global_id, "", insert);
	}

	SmallVector<Value*, 8> input_offsets;
	SmallVector<Value*, 8> output_offsets;

	for (int i=0; i<rtk->nins; i++) {
	    Value *a = new AllocaInst(Type::getInt32Ty(getCLContext()), 0, 4, "ins_idx", global_id);
	    unsigned inner_offset = 0;
	    for (int j=0; j<i; j++) {
		skir_stream_t *s = (skir_stream_t*)rtk->impl_ins[j];
		inner_offset += s->pop_rate;
	    }
	    Value *v = BinaryOperator::Create(BinaryOperator::Add, in_global_offset,
					      ConstantInt::get(Type::getInt32Ty(getCLContext()),
							       inner_offset), "", insert);
	    new StoreInst(v, a, false, insert);
	    input_offsets.push_back(a);
	}
	for (int i=0; i<rtk->nouts; i++) {
	    Value *a = new AllocaInst(Type::getInt32Ty(getCLContext()), 0, 4, "outs_idx",global_id);
	    unsigned inner_offset = 0;
	    for (int j=0; j<i; j++) {
		skir_stream_t *s =  (skir_stream_t*)rtk->impl_ins[j];
		inner_offset += s->push_rate;
	    }
	    Value *v = BinaryOperator::Create(BinaryOperator::Add, out_global_offset,
					      ConstantInt::get(Type::getInt32Ty(getCLContext()),
							       inner_offset), "", insert);
	    new StoreInst(v, a, false, insert);
	    output_offsets.push_back(a);
	}

	// inline the push/pop operations
	//
	for (inst_iterator I = inst_begin(rtk->work), E = inst_end(rtk->work); I != E; ) {
	    Instruction *inst = &*I;
	    ++I;
	    if (isa<SKIRPushInst>(inst)) {
		ConstantInt *CI = cast<ConstantInt>(inst->getOperand(1));
		unsigned id = CI->getZExtValue();

		// XXX this needs to get determined properly
		const Type *elemTy = Type::getInt32Ty(getCLContext());

		Value *off = new LoadInst(output_offsets[id], "", inst);

		Value *out = GetElementPtrInst::Create(arg_outs, off, "", inst);
		out = new BitCastInst(out, PointerType::get(elemTy,1), "", inst);

		Value *elem = new BitCastInst(inst->getOperand(2), 
					      PointerType::get(elemTy,0), "", inst);
		elem = new LoadInst(elem, "", inst);
		
		new StoreInst(elem, out, false, inst);
		
		// update offset
		off = BinaryOperator::Create(BinaryOperator::Add, off,
					     ConstantInt::get(Type::getInt32Ty(getCLContext()),4),
					     "", inst);
		new StoreInst(off, output_offsets[id], false, inst);
		inst->eraseFromParent();
	    }
	    else if (isa<SKIRPopInst>(inst)) {
		ConstantInt *CI = cast<ConstantInt>(inst->getOperand(1));
		unsigned id = CI->getZExtValue();

		// XXX this needs to get determined properly
		const Type *elemTy = Type::getInt32Ty(getCLContext());

		Value *off = new LoadInst(input_offsets[id], "", inst);

		Value *inptr = GetElementPtrInst::Create(arg_ins, off, "", inst);
		inptr = new BitCastInst(inptr, PointerType::get(elemTy,1), "", inst);

		// perform pop
		Value *elem = new LoadInst(inptr, "", inst);

		Value *dst = new BitCastInst(inst->getOperand(2), 
					     PointerType::get(elemTy,0), "", inst);

		new StoreInst(elem, dst, false, inst);
		
		// update offset
		off = BinaryOperator::Create(BinaryOperator::Add, off,
					     ConstantInt::get(Type::getInt32Ty(getCLContext()),4),
					     "", inst);
		new StoreInst(off, input_offsets[id], false, inst);
		inst->eraseFromParent();
	    }	
	}

	// generate the opencl string and run the opencl 
	// backend to get the compiled opencl kernel
	//
	if (verbose) errs() << "\nInput to CL backend:\n\n" << *rtk->work << "\n";

	std::string str;
	runOpenCLBackend(rtk, str);
	if (verbose) errs() << "\nOutput from CL backend:\n\n" << str << "\n";

	// replace kernel state and work function with wrappers
	rtk->state = (void *)new_state;
	rtk->workfn = (work_function*)__SKIRRT_opencl_workfn;
	
#if 0
	std::ifstream file("/home/jfifield/skir-svn/skir/test/nbody/kernel.cl");
	std::string prog(std::istreambuf_iterator<char>(file),(std::istreambuf_iterator<char>()));
	if (verbose) errs() << "\nOutput from CL file:\n\n" << prog << "\n";
	cl::Program::Sources source(1, std::make_pair(prog.c_str(), prog.length()+1));
#else
	cl::Program::Sources source(1, std::make_pair(str.c_str(), str.length()));
#endif

	cl_int err;
	cl::Program program(*context, source);
	err = program.build(devices,"-cl-single-precision-constant -cl-opt-disable");
	checkErr(err, "Program::build()");

	cl::string buildinfo;
	err = program.getBuildInfo(devices[0], CL_PROGRAM_BUILD_LOG, &buildinfo);
	checkErr(err, "Program::getBuildInfo()");
	if (verbose) errs() << buildinfo.c_str() << "\n";

	new_state->cl_kernel = new cl::Kernel(program, rtk->work->getName().str().c_str(), &err);
	checkErr(err, "Kernel::Kernel()");

	new_state->stateB = new cl::Buffer(*context, CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, 
					   new_state->orig_state_size, new_state->orig_state, &err);
	checkErr(err, "new stateB");

	new_state->returnB = new cl::Buffer(*context, CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR, 
					    sizeof(int), &new_state->kernel_return, &err);
	checkErr(err, "new returnB");

	new_state->insB = new cl::Buffer(*context, CL_MEM_READ_ONLY|CL_MEM_ALLOC_HOST_PTR, 
					 OPENCL_BUFFER_SIZE, new_state->in_buf, &err);
	checkErr(err, "new insB");

	new_state->devinsB = new cl::Buffer(*context, CL_MEM_READ_ONLY|CL_MEM_ALLOC_HOST_PTR, 
					 OPENCL_BUFFER_SIZE, new_state->in_buf, &err);
	checkErr(err, "new devinsB");

	new_state->outsB = new cl::Buffer(*context, CL_MEM_WRITE_ONLY|CL_MEM_ALLOC_HOST_PTR, 
					  OPENCL_BUFFER_SIZE, new_state->out_buf, &err);
	checkErr(err, "new outsB");
	new_state->devoutsB = new cl::Buffer(*context, CL_MEM_WRITE_ONLY,
					  OPENCL_BUFFER_SIZE, new_state->out_buf, &err);
	checkErr(err, "new devoutsB");
	
	new_state->in_buf = (char*)queue->enqueueMapBuffer(*new_state->insB, 
							   CL_TRUE, CL_MAP_READ|CL_MAP_WRITE,
							   0, OPENCL_BUFFER_SIZE, NULL, NULL, &err);
	new_state->out_buf = (char*)queue->enqueueMapBuffer(*new_state->outsB,
							    CL_TRUE, CL_MAP_READ|CL_MAP_WRITE,
							    0, OPENCL_BUFFER_SIZE, NULL,NULL, &err);

    }
}

void
SKIROpenCLSched::callKernel(SKIRRuntimeKernel *rtk)
{
    if (!rtk->fixed_sched && rtk->base_work->getName().find("skircl") == std::string::npos) {
	// pass to host scheduler
	sg->getRuntime().getSched()->callKernel(rtk);
    }
    else {
	// internal scheduler
	the_single_sched->callKernel(rtk);
    }
}

void
SKIROpenCLSched::removeKernel(SKIRRuntimeKernel *rtk)
{
    if (!rtk->fixed_sched && rtk->base_work->getName().find("skircl") == std::string::npos) {
	// pass to host scheduler
	sg->getRuntime().getSched()->removeKernel(rtk);
    }
    else {
	// internal scheduler
	the_single_sched->removeKernel(rtk);
    } 
}

void
SKIROpenCLSched::waitKernel(SKIRRuntimeKernel *rtk)
{
    if (!rtk->fixed_sched && rtk->base_work->getName().find("skircl") == std::string::npos) {
	// pass to host scheduler
	sg->getRuntime().getSched()->waitKernel(rtk);
    }
    else {
	// internal scheduler
	the_single_sched->waitKernel(rtk);
    }
}

void
SKIROpenCLSched::pauseKernel(SKIRRuntimeKernel *rtk)
{
    if (!rtk->fixed_sched && rtk->base_work->getName().find("skircl") == std::string::npos) {
	// pass to host scheduler
	sg->getRuntime().getSched()->pauseKernel(rtk);
    }
    else {
	// internal scheduler
	the_single_sched->pauseKernel(rtk);
    }
}

void
SKIROpenCLSched::unPauseKernel(SKIRRuntimeKernel *rtk)
{
    if (!rtk->fixed_sched && rtk->base_work->getName().find("skircl") == std::string::npos) {
	// pass to host scheduler
	sg->getRuntime().getSched()->unPauseKernel(rtk);
    }
    else {
	// internal scheduler
	the_single_sched->unPauseKernel(rtk);
    }
}

void
SKIROpenCLSched::start(void)
{
    if (running.compare_and_swap(1,0) == 0) {
	cl_int err = CL_SUCCESS;

	the_single_sched->start();

	cl::vector<cl::Platform> platformList;
	cl::Platform::get(&platformList);
	if (verbose) errs() << "Number of platforms is: " << platformList.size() << "\n";
    
	cl::string platformName;
	platformList[0].getInfo(CL_PLATFORM_NAME, &platformName);
	if (verbose) errs() << "Platform 0: " << platformName.c_str() << "\n";
    
	// create a context
	cl_context_properties cprops[3] = 
	    {CL_CONTEXT_PLATFORM, (cl_context_properties)(platformList[0])(), 0};
	context = new cl::Context(CL_DEVICE_TYPE_GPU, cprops, NULL, NULL, &err);
	checkErr(err, "Context::Context()");

	// get a list of devices from the context
	devices = context->getInfo<CL_CONTEXT_DEVICES>();
	if (verbose) errs() << "Number of devices is: " << devices.size() << "\n";

	for (unsigned i=0; i<devices.size(); i++) {
	    cl::string deviceName;
	    err = devices[i].getInfo(CL_DEVICE_NAME, &deviceName);
	    checkErr(err, "device getInfo");
	    if (verbose) errs() << "Device " << i << ": " << deviceName.c_str() << "\n";
	}

	// create a command queue

	queue = new cl::CommandQueue(*context, devices[0], SKIRCL_QUEUE_PROFILING_ENABLE, &err);
	checkErr(err, "CommandQueue::CommandQueue()");

	//errs() << "opencl buffer size: " << OPENCL_BUFFER_SIZE << "\n";
    }
}

void
SKIROpenCLSched::stop(void)
{
    if (running.compare_and_swap(0,1) == 1)
	the_single_sched->stop();
}

}
#endif
