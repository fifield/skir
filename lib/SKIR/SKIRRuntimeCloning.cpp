#include <llvm/Pass.h>
#include <llvm/Instructions.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Analysis/Verifier.h>

#include <SKIR/SKIRRuntime.h>
#include "SKIRRuntimeKernel.h"

// these FunctionPasses modify the module, which is not allowed, so they 
// shouldn't be run with opt or other from other llvm code

using namespace llvm;

namespace {

struct SKIRCloneWorkPass : public FunctionPass {

    static char ID;
    SKIRRuntimeKernel *kernel;
    Module *dst_mod;

    SKIRCloneWorkPass() 
	: FunctionPass((intptr_t)&ID), kernel(NULL), dst_mod(NULL)
    {
    }

    SKIRCloneWorkPass(SKIRRuntimeKernel *k, Module *dstMod = NULL)
	: FunctionPass((intptr_t)&ID), kernel(k), dst_mod(dstMod)
    {
    }

    bool runOnFunction(Function &F)
    {
	Function *work = &F;
	Module *src_mod = work->getParent();
	if (!dst_mod) dst_mod = src_mod;

	Function *f = src_mod->getFunction("__SKIRRT_workfn");
	assert(f);
	Function::const_arg_iterator arg_it = f->arg_begin();
	const Type *rt_state_type = arg_it->getType();
	arg_it++; arg_it++;  //advance to stream arrays

	std::vector<const Type*> arg_types;
	Function::const_arg_iterator old_work_arg = work->arg_begin();
	arg_types.push_back(rt_state_type); // rt state
	arg_types.push_back((old_work_arg++)->getType()); // kernel state
	arg_types.push_back((arg_it++)->getType());  // ins
	arg_types.push_back((arg_it)->getType());  // outs
	
	FunctionType *work_type = FunctionType::get(f->getFunctionType()->getReturnType(),
						   arg_types, false);
	Function *new_work = Function::Create(work_type, work->getLinkage(), 
					      work->getName()+"_rt");
	
	DenseMap<const Value*, Value*> value_map;
	Function::arg_iterator new_work_arg = new_work->arg_begin();
	new_work_arg->setName("rt_state");
	new_work_arg++;
	
	old_work_arg = work->arg_begin();
	new_work_arg->setName(old_work_arg->getName());
	value_map[old_work_arg++] = new_work_arg++;
	new_work_arg->setName(old_work_arg->getName());
	value_map[old_work_arg++] = new_work_arg++;
	new_work_arg->setName(old_work_arg->getName());
	value_map[old_work_arg++] = new_work_arg++;
	
	SmallVector<ReturnInst*, 8> ret;
	CloneFunctionInto(new_work, work, value_map, ret, "", 0);

	dst_mod->getFunctionList().push_back(new_work);

	for (inst_iterator I = inst_begin(new_work), E = inst_end(new_work); I != E; ) {
	    Instruction *inst = &*I;
	    ++I;
	    if (ReturnInst *RI = dyn_cast<ReturnInst>(inst)) {
		if (RI->getOperand(0)->getType() != f->getFunctionType()->getReturnType()) {
		    Value *ret = new IntToPtrInst(RI->getReturnValue(),
						  f->getFunctionType()->getReturnType(),
						  RI->getReturnValue()->getName(),
						  RI);
		    /*ReturnInst *newRI =*/ ReturnInst::Create(new_work->getContext(), ret, RI);
		    RI->eraseFromParent();
		}
	    }
	    else if (CallInst *CI = dyn_cast<CallInst>(inst)) {
		if (dyn_cast<Function>(CI->getCalledFunction())) {
		    std::string name(CI->getCalledFunction()->getName());
		    if (!name.find("__SKIRRT_call")) {
                        const Type *vty = 
                            PointerType::get(Type::getInt8Ty(new_work->getContext()),0);
			const Type *ty = PointerType::get(vty,0);
                            //GetVoidPtrPtrType(new_work->getContext());
			Value *v0 = new BitCastInst(CI->getOperand(3), ty, "", CI);
			Value *v1 = new BitCastInst(CI->getOperand(4), ty, "", CI);
			CI->setOperand(3,v0);
			CI->setOperand(4,v1);
		    }
		}
	    }
	}
	if (kernel) kernel->work = new_work;
	return true;
    }
};

char SKIRCloneWorkPass::ID = 0;
RegisterPass<SKIRCloneWorkPass> Y("skir-clone-work",
				  "SKIR Runtime clone work", false, false);
}

namespace llvm {

FunctionPass *createSKIRCloneWorkPass()
{
    return new SKIRCloneWorkPass;
}

}

void
SKIRRuntime::runSKIRCloneWorkPass(SKIRRuntimeKernel *k, Module *dstMod)
{
    Module *mod = k->work->getParent();

    FunctionPassManager PM(mod);
    PM.add(new TargetData(mod));
    PM.add(new SKIRCloneWorkPass(k, dstMod));

    {
	MutexGuard locked(k->cg->lock);
	PM.run(*k->work);
    }
}
