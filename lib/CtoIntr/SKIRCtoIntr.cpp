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
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/CallSite.h>
#include "llvm/Intrinsics.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Constants.h"

using namespace llvm;

namespace {

template <class ArgIt> static CallInst 
*ReplaceCallWith(Function *f, CallInst *CI,
		 ArgIt ArgBegin, ArgIt ArgEnd) {
    
    SmallVector<Value *, 8> Args(ArgBegin, ArgEnd);
    CallInst *NewCI = CallInst::Create(f, Args.begin(), Args.end(), CI->getName(), CI);

    if (!CI->use_empty())
	CI->replaceAllUsesWith(NewCI);
    CI->eraseFromParent();

    return NewCI;
}

struct SKIRCtoIntrPass : public FunctionPass {
    static char ID;
    SKIRCtoIntrPass() : FunctionPass((intptr_t)&ID) {}
    virtual bool runOnFunction(Function &F) {
	Module *mod = F.getParent();
	for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ) {
	    Instruction *inst = &*I;
	    ++I;
	    if (CallInst *CI = dyn_cast<CallInst>(inst)) {
		Function *F = CI->getCalledFunction();
		if (!F) continue;
		//LLVMContext &CTX = F->getContext();
		if (F->getName() == "__SKIR_kernel") {
		    Value *ops[2] = { CI->getOperand(1),   /* work() name */
				      CI->getOperand(2) }; /* arguments */
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_kernel);
		    ReplaceCallWith(f, CI, ops, ops+2);
		}
		else if (F->getName() == "__SKIR_call") {
		    Value *ops[3] = { CI->getOperand(1),   /* RuntimeKernel* */
				      CI->getOperand(2),   /* ins */
				      CI->getOperand(3) }; /* outs */
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_call);
		    ReplaceCallWith(f, CI, ops, ops+3);
		}
		else if (F->getName() == "__SKIR_wait") {
		    Value *ops[1] = { CI->getOperand(1) };  /* kernel* */
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_wait);
		    ReplaceCallWith(f, CI, ops, ops+1);
		}
		else if (F->getName() == "__SKIR_stream") {
		    Value *ops[1] = { CI->getOperand(1) };
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_stream);
		    ReplaceCallWith(f, CI, ops, ops+1);
		}
		else if (F->getName() == "__SKIR_push") {
		    Value *ops[2] = { CI->getOperand(1),   /* stream */
				      CI->getOperand(2) }; /* elm */
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_push);
		    ReplaceCallWith(f, CI, ops, ops+2);
		}
		else if (F->getName() == "__SKIR_pop") {
		    Value *ops[2] = { CI->getOperand(1),   /* stream */
				      CI->getOperand(2) }; /* elmptr */
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_pop);
		    ReplaceCallWith(f, CI, ops, ops+2);
		}
		else if (F->getName() == "__SKIR_peek") {
		    Value *ops[3] = { CI->getOperand(1),   /* stream */
				      CI->getOperand(2),   /* elmptr */
				      CI->getOperand(3) }; /* offset */
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_peek);
		    ReplaceCallWith(f, CI, ops, ops+3);
		}
		else if (F->getName() == "__SKIR_rdtsc") {
		    Value *ops[1] = {0};
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::readcyclecounter);
		    ReplaceCallWith(f, CI, ops, ops);
		}
		else if (F->getName() == "__SKIR_prefetch") {
		    Value *ops[3] = { CI->getOperand(1),   /* address */
				      CI->getOperand(2),   /* rw */
				      CI->getOperand(3) }; /* locality */
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::prefetch);
		    ReplaceCallWith(f, CI, ops, ops+3);
		}
		else if (F->getName() == "__SKIR_yield") {
		    Value *ops[2] = { CI->getOperand(1), CI->getOperand(2) };
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_yield);
		    ReplaceCallWith(f, CI, ops, ops+2);
		}
		else if (F->getName() == "__SKIR_return") {
		    Value *ops[2] = { CI->getOperand(1), CI->getOperand(2) };
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_return);
		    ReplaceCallWith(f, CI, ops, ops+2);
		}
		else if (F->getName() == "__SKIR_become") {
		    LLVMContext &CTX = F->getContext();
		    const PointerType *voidPtrTy = PointerType::get(Type::getInt8Ty(CTX),0);
		    Value *ops[2] = { ConstantPointerNull::get(voidPtrTy),
				      CI->getOperand(1) };
		    Function *f = Intrinsic::getDeclaration(mod, Intrinsic::skir_become);
		    ReplaceCallWith(f, CI, ops, ops+2);
		}
	    }
	}
	return true;
    }
    
private:
};

char SKIRCtoIntrPass::ID = 0;
RegisterPass<SKIRCtoIntrPass> X("skir-c-to-intr",
				"Convert C to SKIR Intrinisics", false, false);

}

namespace llvm {

FunctionPass *createSKIRCtoIntrPass()
{
    return new SKIRCtoIntrPass;
}

}
