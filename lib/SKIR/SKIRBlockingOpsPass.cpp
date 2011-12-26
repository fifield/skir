#include <llvm/Pass.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Function.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Constants.h>
#include <llvm/IntrinsicInst.h>

#include <SKIR/SKIRRuntime.h>

#include "SKIRRuntimeStream.h"
#include "SKIRUtil.h"

using namespace llvm;


namespace {

struct SKIRBlockingOpsPass : public FunctionPass 
{
private:
    SKIRRuntimeKernel *kernel;
    
public:

    static char ID;
    SKIRBlockingOpsPass() : FunctionPass((intptr_t)&ID) {}
    SKIRBlockingOpsPass(SKIRRuntimeKernel *k) : FunctionPass((intptr_t)&ID), kernel(k)  {}

    /// replace skir intrinsics with blocking runtime calls:
    ///   skir.OP -> __SKIRRT_inline_OP_block
    bool runOnFunction(Function &work)
    {
	assert(kernel && kernel->rt_ins && kernel->rt_outs);

	Function::arg_iterator args = work.arg_begin();
	args++; args++;
	Value *vins = args++;
	Value *vouts = args;

	std::string inline_suffix("_block");

	for (inst_iterator I=inst_begin(work),E=inst_end(work); I!=E; ) {
	    Instruction *p = &*I; ++I;
	    if (CallInst *CI = dyn_cast<CallInst>(p)) {
		if (isa<SKIRPushInst>(p)) {
		    Value *ops[3] = { vouts, p->getOperand(1), p->getOperand(2) };
		    Constant *F = getInlineCode(work.getParent(),
						("__SKIRRT_inline_push"+inline_suffix).c_str());
		    CI = ReplaceCallWith(CI, F, ops, ops+3);
		}
		else if (isa<SKIRPopInst>(p)) {
		    Value *ops[3] = { vins, p->getOperand(1), p->getOperand(2) };
		    Constant *F = getInlineCode(work.getParent(),
						("__SKIRRT_inline_pop"+inline_suffix).c_str());
		    CI = ReplaceCallWith(CI, F, ops, ops+3);
		}
		else if (isa<SKIRPeekInst>(p)) {
		    Value *ops[4] = { vins, p->getOperand(1), p->getOperand(2), p->getOperand(3) };
		    Constant *F = getInlineCode(work.getParent(),
						("__SKIRRT_inline_peek"+inline_suffix).c_str());
		    CI = ReplaceCallWith(CI, F, ops, ops+4);
		}
	    }
	}

	return true;
    }

};

char SKIRBlockingOpsPass::ID = 0;
RegisterPass<SKIRBlockingOpsPass> X("skir-blocking-ops",
                                    "SKIR - inline blocking stream operations", false, false);

}

namespace llvm {

    FunctionPass *createSKIRBlockingOpsPass()
    {
	return new SKIRBlockingOpsPass;
    }

    void
    SKIRRuntime::addSKIRBlockingOpsPass(SKIRRuntimeKernel *k)
    {
	k->fpm->add(new SKIRBlockingOpsPass(k));
	k->fpm->add(createVerifierPass());
    }

    void
    SKIRRuntime::runSKIRBlockingOpsPass(SKIRRuntimeKernel *k)
    {
	// run the pass
	Function *F = k->work;
	FunctionPassManager PM(F->getParent());

	PM.add(new TargetData(F->getParent()));
	PM.add(new SKIRBlockingOpsPass(k));
	PM.add(createVerifierPass());

	{
	    MutexGuard locked(k->cg->lock);
	    PM.run(*F);
	}
    }

}
