#include <llvm/Pass.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Function.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Constants.h>
#include <llvm/IntrinsicInst.h>
//#include <llvm/Instructions.h>
//#include <llvm/Type.h>

#include <SKIR/SKIRRuntime.h>

#include "SKIRRuntimeStream.h"
//#include "SKIRKernelInfoPass.h"
#include "SKIRUtil.h"

using namespace llvm;


namespace {

struct SKIROuterLoopPass : public FunctionPass 
{
public:
    SKIRRuntimeKernel *kernel;
    std::string workfn_suffix;
    std::string ops_suffix;

    static char ID;
    SKIROuterLoopPass() : FunctionPass((intptr_t)&ID) {}
    SKIROuterLoopPass(SKIRRuntimeKernel *k, const char *workfn, const char *ops)
	: FunctionPass((intptr_t)&ID), kernel(k)
    {
	workfn_suffix = workfn;
	ops_suffix = ops;
    }

    bool runOnFunction(Function &work)
    { 
	assert(kernel && kernel->rt_ins && kernel->rt_outs);
	
	Function::arg_iterator args = work.arg_begin();
	args++; args++;
	Value *vins = args++;
	Value *vouts = args;

	if (ops_suffix == "nocheck") {
	    if ((kernel->nins == 1) && (kernel->nouts == 1))
		ops_suffix += std::string("_p");
	}
	
	if (ops_suffix.length()) {
	    ops_suffix = "_" + ops_suffix;
	}
	
	// replace stream ops
	for (inst_iterator I=inst_begin(work),E=inst_end(work); I!=E; ) {
	    Instruction *p = &*I; ++I;
	    if (CallInst *CI = dyn_cast<CallInst>(p)) {
		if (isa<SKIRPushInst>(p)) {
		    Value *ops[3] = { vouts, p->getOperand(1), p->getOperand(2) };
		    Constant *F = getInlineCode(work.getParent(),
						("__SKIRRT_inline_push"+ops_suffix).c_str());
		    CI = ReplaceCallWith(CI, F, ops, ops+3);
		}
		else if (isa<SKIRPopInst>(p)) {
		    Value *ops[3] = { vins, p->getOperand(1), p->getOperand(2) };
		    Constant *F = getInlineCode(work.getParent(), 
						("__SKIRRT_inline_pop"+ops_suffix).c_str());
		    CI = ReplaceCallWith(CI, F, ops, ops+3);
		}
		else if (isa<SKIRPeekInst>(p)) {
		    Value *ops[4] = { vins, p->getOperand(1), p->getOperand(2), 
				      p->getOperand(3) };
		    Constant *F = getInlineCode(work.getParent(), 
						("__SKIRRT_inline_peek"+ops_suffix).c_str());
		    CI = ReplaceCallWith(CI, F, ops, ops+4);
		}
	    }
	}

	// create new work fn
	Function *new_work;

	std::stringstream name;
	if (workfn_suffix == "nocheck") {
	    // find out if all the streams have the same element size.
	    // if they do, we can specialize the workfn for that size
	    bool equal = true;
	    size_t elem_size = 0;
	    for (int i=0; i<kernel->nins && equal; i++)
		if (!elem_size) elem_size = kernel->rt_ins[i]->elem_size;
		else equal = equal && (elem_size == kernel->rt_ins[i]->elem_size);
	    for (int i=0; i<kernel->nouts && equal; i++)
		if (!elem_size) elem_size = kernel->rt_outs[i]->elem_size;
		else equal = equal && (elem_size == kernel->rt_outs[i]->elem_size);

	    name << "__SKIRRT_workfn_nocheck";
	    if ((kernel->nins == 1) && (kernel->nouts == 1)) {
		name << "_1_1";
	    } 
	    else if (equal) {
		name << "_" << elem_size << "_" << STREAM_BUFFER_SIZE;
	    }
	}
	else {
	    name << "__SKIRRT_workfn_" + workfn_suffix;
	}
	new_work = makeWorkFromSkel(kernel, name.str().c_str());

	// set the arguments of any calls to __SKIRRT_inline_compute_niters
	for (inst_iterator I=inst_begin(new_work),E=inst_end(new_work); I!=E; ) {
	    Instruction *p = &*I; ++I;
	    if (CallInst *CI = dyn_cast<CallInst>(p)) {
		if (Function *F = dyn_cast<Function>(CI->getCalledFunction())) {
		    std::string name(F->getName());
		    if (name.find("__SKIRRT_inline_compute_niters") != std::string::npos) {
			// __SKIRRT_inline_compute_niters(&niter, ins, 1, outs, 1);
			CI->setOperand(3, ConstantInt::get(Type::getInt32Ty(CI->getContext()), 
							   kernel->nins));
			CI->setOperand(5, ConstantInt::get(Type::getInt32Ty(CI->getContext()), 
							   kernel->nouts));
		    }
		}
	    }
	}

	kernel->work->deleteBody();
	
	// replace the body of the old work function with 
	// the body of the new work function
	Function::arg_iterator old_args = work.arg_begin();
	Function::arg_iterator new_args = new_work->arg_begin();
	DenseMap<const Value*, Value*> value_map;
	for (unsigned i=0; i<work.arg_size(); i++)
	    value_map[new_args++] = old_args++;

	// clone new work into old work
	SmallVector<ReturnInst*, 8> rets;
	CloneFunctionInto(&work, new_work, value_map, rets, "", 0);

	delete new_work;
	return true;
    }
};
char SKIROuterLoopPass::ID = 0;
RegisterPass<SKIROuterLoopPass> X("skir-outer-loop",
				  "SKIR Kernel outer loop pass", false, false);

}

namespace llvm {
 
FunctionPass *createSKIROuterLoopPass()
{
    return new SKIROuterLoopPass;
}

void
SKIRRuntime::addSKIROuterLoopPass(SKIRRuntimeKernel *k, const char *c, const char *d)
{
    k->fpm->add(new SKIROuterLoopPass(k, c, d));
    k->fpm->add(createVerifierPass());
}

void
SKIRRuntime::runSKIROuterLoopPass(SKIRRuntimeKernel *k, const char *c, const char *d)
{
    // run the pass
    Function *F = k->work;
    FunctionPassManager PM(F->getParent());
	
    PM.add(new TargetData(F->getParent()));
    PM.add(new SKIROuterLoopPass(k, c, d));
    PM.add(createVerifierPass());

    {
	MutexGuard locked(k->cg->lock);
	PM.run(*F);
    }
}

}
