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

#include <llvm/Pass.h>
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Constants.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include "llvm/Support/CommandLine.h"

#include <SKIR/SKIRRuntime.h>
#include "SKIRRuntimeStream.h"
#include "SKIRRuntimeKernel.h"
#include "SKIRUtil.h"

#include <vector>
#include <string>

using namespace llvm;

namespace {

struct SKIRInlineStreamsSelector {
    std::vector<int> input_types;
    std::vector<int> output_types;
};

static cl::opt<bool>
NoInlineStreamOps("no-inline-stream-ops", cl::desc("dont inline push/peek/pop"));

struct SKIRInlineStreamsPass : public FunctionPass {

 private:
    //    SKIRInlineStreamsSelector *sel;
    SKIRRuntimeKernel *kernel;

    Module *mod;
    Module *inline_module;

 public:    
    //    void setSelector(SKIRInlineStreamsSelector *s) { sel = s; }

    static char ID;
    SKIRInlineStreamsPass() : FunctionPass((intptr_t)&ID)/*, sel(0)*/ { 
	inline_module = 0;
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
     }
    
    void setKernel(SKIRRuntimeKernel *k) {
	kernel = k;
    }
    
    virtual bool runOnFunction(Function &F) {
	Value *vins;
	Value *vouts;
	std::vector<CallInst*> inline_sites;
	LLVMContext &CTX = F.getContext();
	Function *work = &F;
	mod = F.getParent();
	assert(mod && "F is required to have a parent module");

#if 0
	// if the entry block doesn't end in a switch
	bool ends_in_switch = true;
	TerminatorInst *TI1, *TI0 = work->getEntryBlock().getTerminator();
	if (TI0->getNumSuccessors() != 2) {
	    ends_in_switch = false;
	} else {
	    TI1 = TI0->getSuccessor(1)->getTerminator();
	    if (!isa<SwitchInst>(TI1))
		ends_in_switch = false;
	}
#endif

	// first pass: replace intrinsics with function names, record the callsites
	Function::arg_iterator args = work->arg_begin();
	args++;	args++;
	vins = args++;
	vouts = args;

	// record inline sites
	for (Function::iterator BI = work->begin(), BE = work->end(); BI != BE; ) {
	    BasicBlock *blk = BI; ++BI;
	    for (BasicBlock::iterator I = blk->begin(), E = blk->end(); I != E; ) {
		Instruction *inst = &*I; ++I;
	    
		if (CallInst *CI = dyn_cast<CallInst>(inst)) {
		    if (isa<SKIRIntrinsic>(inst)) {
			//assert(ends_in_switch);
			CallInst *newCI = 0;
			bool do_inline = true;
			if (isa<SKIRPushInst>(inst)) {
			    Value *ops[3] = { vouts,
					      CI->getOperand(1),   /* stream */
					      CI->getOperand(2) }; /* elm */
			    static Constant *pushFCache = 0;
			    if (!pushFCache) 
				pushFCache = getInlineCode(mod, "__SKIRRT_inline_push");
			    newCI = ReplaceCallWith("__SKIRRT_inline_push", 
						    CI, ops, ops+3, Type::getVoidTy(CTX),
						    pushFCache);
			}
			else if (isa<SKIRPopInst>(inst)) {
			    Value *ops[3] = { vins,
					      CI->getOperand(1),   /* stream */
					      CI->getOperand(2) }; /* elmptr */
			    static Constant *popFCache;
			    if (!popFCache) 
				popFCache = getInlineCode(mod, "__SKIRRT_inline_pop");
			    newCI = ReplaceCallWith("__SKIRRT_inline_pop",
						    CI, ops, ops+3, Type::getVoidTy(CTX),
						    popFCache);
			}
			else if (isa<SKIRPeekInst>(inst)) {
			    static Constant *peekFCache;
			    if (!peekFCache) 
				peekFCache = getInlineCode(mod, "__SKIRRT_inline_peek");
			    Value *ops[4] = { vins,
					      CI->getOperand(1),   /* stream */
					      CI->getOperand(2),   /* elmptr */
					      CI->getOperand(3) }; /* offset */
			    newCI = ReplaceCallWith("__SKIRRT_inline_peek",
						    CI, ops, ops+4, Type::getVoidTy(CTX),
						    peekFCache);
			}
			else if (isa<SKIRBecomeInst>(inst)) {
			    static Constant *becomeFCache;
			    if (!becomeFCache) becomeFCache = mod->getFunction("__SKIRRT_become");
			    Value *ops[4] = { CI->getOperand(1),   /* rt */
					      CI->getOperand(2),   /* kernels */
					      vins,
					      vouts };
			    newCI = ReplaceCallWith("__SKIRRT_become",
						    CI, ops, ops+4, Type::getVoidTy(CTX), 
						    becomeFCache);
			    
			    do_inline = false;
			    BasicBlock::iterator SplitIt = &*I;
			    BasicBlock *BB = newCI->getParent()->splitBasicBlock(SplitIt);
			    BI = BB;
			    BE = work->end();
			    I = BB->getInstList().begin();
			    E = BB->getInstList().end();

			    Instruction *TI = newCI->getParent()->getTerminator();
			    Value *retVal = ConstantInt::get(Type::getInt32Ty(CTX), 1);
			    retVal = new IntToPtrInst(retVal, Type::getInt8PtrTy(CTX,0), "", TI);
			    /*Instruction *RI =*/ ReturnInst::Create(CTX, retVal, TI);
			    TI->eraseFromParent();
			}
			else {
			    inst->dump();
			    assert(0 && "unexpected SKIR instruction");
			}
			assert(newCI);
			if (do_inline) inline_sites.push_back(newCI);
		    }
		    else {
			if (!CI->getCalledFunction()) continue;
			std::string name(CI->getCalledFunction()->getName());
			if (!NoInlineStreamOps) {
			    if (
				(name.find("__SKIRRT_inline_push") != std::string::npos) ||
				(name.find("__SKIRRT_inline_peek") != std::string::npos) ||
				(name.find("__SKIRRT_inline_pop") != std::string::npos) ||
				(name.find("__SKIRRT_inline_array") != std::string::npos)// ||
				//(name.find("__SKIRRT_inline_compute_niters_1_1") != std::string::npos)||
				//(name.find("__SKIRRT_inline_compute_niters") != std::string::npos)
				)
				inline_sites.push_back(CI);
			}
		    }
		}
	    }
	}

	// do inlines
	std::vector<CallInst*>::iterator iter;
	for (iter = inline_sites.begin(); iter != inline_sites.end(); iter++) {
	    Instruction *I = *iter;
	    assert( InlineFunction(I) );
	}

#if 0
	// replace SKIRRT_would_block
	for (inst_iterator I = inst_begin(work), E = inst_end(work); I != E;) {
	    Instruction &inst = *I;
	    ++I;
	    
	    if (isa<CallInst>(inst)) {
		CallInst &CI = cast<CallInst>(inst);
		Function *CF = CI.getCalledFunction();
		if (!CF) continue;
		if (CF->getName() == "__SKIRRT_would_block") {
		    Instruction &nexti = *I;

		    Value *ret = CI.getOperand(1);

		    if (kernel && kernel->opt_only) {
			if ( isa<TerminatorInst>(nexti) ) {
			    ++I;
			    nexti.eraseFromParent();
			}
			ReplaceInstWithInst(&inst, ReturnInst::Create(CTX, ret));
			break;
		    }
		    else {
			// get rid of the existing block terminator
			if ( isa<TerminatorInst>(nexti) ) {
			    ++I;
			    nexti.eraseFromParent();
			}
			
			// XXX here 
			// replace dummy call with branch to entry.save block
			BasicBlock *entry_save = TI1->getSuccessor(1);
			BranchInst *BI = BranchInst::Create(entry_save);
			
			ReplaceInstWithInst(&CI, BI); 
			
			ReturnInst *ret_orig = cast<ReturnInst>(entry_save->getTerminator());
			PHINode *phi = dyn_cast<PHINode>(ret_orig->getOperand(0));
			if (!phi) {
			    phi = PHINode::Create(ret->getType(), "ret", 
						  entry_save->getFirstNonPHI());
			}
			phi->addIncoming(ret, BI->getParent());
			ReplaceInstWithInst(ret_orig, ReturnInst::Create(CTX,phi));
		    }
		}
	    }
	}	

	// if it lands fakie
	if (ends_in_switch) {
	    SwitchInst *SI = cast<SwitchInst>(TI1);
	    SI->removeCase(1);
	}
#endif	
	return true;
    }
};
 
 char SKIRInlineStreamsPass::ID = 0;
 RegisterPass<SKIRInlineStreamsPass> X("skir-inline-streams",
				       "SKIR Stream Operation Inlining", false, false);

}

namespace llvm {

FunctionPass *createSKIRInlineStreamsPass()
{
    return new SKIRInlineStreamsPass;
}

}

void
SKIRRuntime::addSKIRInlineStreamsPass(SKIRRuntimeKernel *kernel)
{
    SKIRInlineStreamsPass *inlinePass = new SKIRInlineStreamsPass;
    inlinePass->setKernel(kernel);

    kernel->fpm->add(inlinePass);
    kernel->fpm->add(createVerifierPass());
}

void
SKIRRuntime::runSKIRInlineStreamsPass(SKIRRuntimeKernel *k)
{
    Function *F = k->work;

    // run the pass 
    {
	FunctionPassManager PM(F->getParent());

	PM.add(new TargetData(F->getParent()));
	
	SKIRInlineStreamsPass *inlinePass = new SKIRInlineStreamsPass;
	inlinePass->setKernel(k);

	PM.add(inlinePass);
	PM.add(createVerifierPass());

	{
	    MutexGuard locked(k->cg->lock);
	    PM.run(*F);
	}
    }
}
