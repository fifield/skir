#include <llvm/Pass.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Function.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Constants.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Instructions.h>

#include <SKIR/SKIRRuntime.h>

#include "SKIRRuntimeStream.h"
#include "SKIRUtil.h"

using namespace llvm;


namespace {

struct SKIRKoroPass : public FunctionPass 
{
public:
    SKIRRuntimeKernel *kernel;

    static char ID;
    SKIRKoroPass() : FunctionPass((intptr_t)&ID) {}
    SKIRKoroPass(SKIRRuntimeKernel *k) : FunctionPass((intptr_t)&ID), kernel(k)  {}

    bool runOnFunction(Function &work)
    { 
	assert(kernel && kernel->rt_ins && kernel->rt_outs);
	
	Function::arg_iterator args = work.arg_begin();
	Value *rt_state = args;
	bool ret = false;
	Module *mod = work.getParent();
	LLVMContext &CTX = work.getContext();

	// replace SKIRRT_would_block
	for (inst_iterator I = inst_begin(work), E = inst_end(work); I != E;) {
	    Instruction &inst = *I;
	    ++I;
	    
	    if (isa<CallInst>(inst)) {
		CallInst &CI = cast<CallInst>(inst);
		Function *CF = CI.getCalledFunction();
		if (!CF) continue;
		if (CF->getName() == "__SKIRRT_would_block") {
		    Value *from = CI.getOperand(1);
		    Value *to = CI.getOperand(2);
		    Value *ops[2] = {from, to};
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_yield);
		    ReplaceCallWith(f, &CI, ops, ops+2);
		    ret = true;
		}
	    }
	    else if (isa<ReturnInst>(inst)) {
		Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_return);
		const Type *voidPtrTy = PointerType::get(Type::getInt8Ty(CTX),0);
		Value *cast_rt_state = new BitCastInst(rt_state, voidPtrTy, "", &inst);
		Value *ops[2] = { new BitCastInst(cast_rt_state, voidPtrTy, "", &inst),
				  new PtrToIntInst(inst.getOperand(0), 
						   Type::getInt32Ty(CTX), "", &inst) };
		CallInst *CI = CallInst::Create(f, ops, ops+2, "", &inst);
		ret = true;
	    }
	}
	return ret;
    }
};
char SKIRKoroPass::ID = 0;
RegisterPass<SKIRKoroPass> X("skir-koro",
			     "SKIR Kernel coroutine", false, false);

}

namespace llvm {
 
FunctionPass *createSKIRKoroPass()
{
    return new SKIRKoroPass;
}

void
SKIRRuntime::addSKIRKoroPass(SKIRRuntimeKernel *k)
{
    k->fpm->add(new SKIRKoroPass(k));
    k->fpm->add(createVerifierPass());
}

void
SKIRRuntime::runSKIRKoroPass(SKIRRuntimeKernel *k)
{
    // run the pass
    Function *F = k->work;
    FunctionPassManager PM(F->getParent());
	
    PM.add(new TargetData(F->getParent()));
    PM.add(new SKIRKoroPass(k));
    PM.add(createVerifierPass());

    {
	MutexGuard locked(k->cg->lock);
	PM.run(*F);
    }
}

}
