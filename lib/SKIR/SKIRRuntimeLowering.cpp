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
#include <llvm/Instructions.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Analysis/Verifier.h>

#include <SKIR/SKIRRuntime.h>
#include "SKIRUtil.h"

using namespace llvm;

namespace {

//
// SKIRLowerGraphOpsPass
//

struct SKIRLowerGraphOpsPass : public ModulePass {
    void *rt;
    bool ret;

public:
    static char ID;
    SKIRLowerGraphOpsPass(void *rt) : ModulePass((intptr_t)&ID), rt(rt) {}
    SKIRLowerGraphOpsPass() : ModulePass((intptr_t)&ID), rt(NULL) {}

    bool runOnModule(Module &M)
    {
	ret = false;
	for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
	    inst_iterator I = inst_begin(MI);
	    inst_iterator E = inst_end(MI);
	    while (I != E) {
		Instruction &inst = *I;
		++I;
		if (CallInst *CI = dyn_cast<CallInst>(&inst)) {
		    visitCallInst(*CI);
		}
	    }
	}
	return ret;
    }
    void visitCallInst(CallInst &CI) {
	Function *F = CI.getCalledFunction();
	if (!F) return; //can be null for function pointer
	LLVMContext &CTX = F->getContext();

	Module *M = CI.getParent()->getParent()->getParent();
	const Type *intPtrType;
	if (M->getPointerSize() == Module::Pointer64)
	    intPtrType = Type::getInt64Ty(CTX);
	else if (M->getPointerSize() == Module::Pointer32)
	    intPtrType = Type::getInt32Ty(CTX);
	else
	    assert(0 && "Could not determine target pointer size");

	// replace these with the runtime implementation
	if (isa<SKIRKernelInst>(&CI)) {
	    static Constant *kernelFCache;
	    Value *me = ConstantInt::get(intPtrType, (uintptr_t)rt);
	    me = new IntToPtrInst(me, Type::getInt8PtrTy(CTX,0), "", &CI);
	    Value *ops[3] = { me,                   /* SKIRRuntime* */
			      CI.getOperand(1),     /* work() name */
			      CI.getOperand(2) };   /* arguments */
	    /* void *__SKIRRT_kernel(void *, void *) */
	    ReplaceCallWith("__SKIRRT_kernel", &CI, ops, ops+3, 
			    PointerType::get(Type::getInt8Ty(CTX),0), kernelFCache);
	    ret = true;
	}
	else if (isa<SKIRCallInst>(&CI)) {
	    static Constant *callFCache;
	    Value *me = ConstantInt::get(intPtrType, (uintptr_t)rt);
	    me = new IntToPtrInst(me, Type::getInt8PtrTy(CTX,0), "", &CI);
	    Value *ops[4] = { me,                 /* SKIRRuntime* */
			      CI.getOperand(1),   /* RuntimeKernel* */
			      CI.getOperand(2),   /* ins */
			      CI.getOperand(3) }; /* outs */
	    ReplaceCallWith("__SKIRRT_call", &CI, ops, ops+4, Type::getVoidTy(CTX), callFCache);
	    ret = true;
	}
	else if (isa<SKIRWaitInst>(&CI)) {
	    static Constant *waitFCache;
	    Value *me = ConstantInt::get(intPtrType, (uintptr_t)rt);
	    Value *ops[2] = { me,                 /* SKIRRuntime* */
			      CI.getOperand(1) }; /* RuntimeKernel* */
	    ReplaceCallWith("__SKIRRT_wait", &CI, ops, ops+2, Type::getVoidTy(CTX), waitFCache);
	    ret = true;
	}
	else if (isa<SKIRBecomeInst>(&CI)) {
	    Value *me = new IntToPtrInst(ConstantInt::get(intPtrType, (uintptr_t)rt), 
					 Type::getInt8PtrTy(CTX,0), "", &CI);
	    CI.setOperand(1, me);

	    //BasicBlock::iterator iter = &CI;
	    //if (*iter == CI.getParent()->getTerminator())
	    //CI.getParent()->splitBasicBlock(++iter);

	    //Instruction *RI = ReturnInst::Create(CTX, ConstantInt::get(Type::getInt32Ty(CTX), 1));
	    //RI->insertAfter(&CI);

	    ret = true;
	}
	else if (isa<SKIRStreamInst>(&CI)) {
	    static Constant *streamFCache;
	    Value *me = ConstantInt::get(intPtrType, (uintptr_t)rt);
	    Value *ops[2] = { me,                 /* SKIRRuntime* */
			      CI.getOperand(1) }; /* elem_size * */
	    ReplaceCallWith("__SKIRRT_stream", &CI, ops, ops+2,
			    PointerType::get(Type::getInt8Ty(CTX),0), streamFCache);
	    ret = true;
	}
    }
};
char SKIRLowerGraphOpsPass::ID = 0;
RegisterPass<SKIRLowerGraphOpsPass> X("skir-lower-graph-ops",
				      "SKIR Runtime LowerGraphOps", false, false);


//
// SKIRLowerStreamOpsPass
//

struct SKIRLowerStreamOpsPass : public FunctionPass {
    void *rt;
    bool ret;

public:
    static char ID;
    SKIRLowerStreamOpsPass(void *rt) : FunctionPass((intptr_t)&ID), rt(rt) {}
    SKIRLowerStreamOpsPass() : FunctionPass((intptr_t)&ID), rt(NULL) {}

    bool runOnModule(Module &M)
    {
	ret = false;
	for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
	    inst_iterator I = inst_begin(MI);
	    inst_iterator E = inst_end(MI);
	    while (I != E) {
		Instruction &inst = *I;
		++I;
		if (CallInst *CI = dyn_cast<CallInst>(&inst)) visitCallInst(*CI);
	    }
	}
	return ret;
    }

    bool runOnFunction(Function &F) {
	ret = false;
	for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ) {
	    Instruction &inst = *I;
	    ++I;
	    if (CallInst *CI = dyn_cast<CallInst>(&inst)) visitCallInst(*CI);
	}
	return ret;
    }

    void visitCallInst(CallInst &CI) {
	Function *F = CI.getCalledFunction();
	if (!F) return; //can be null for function pointer
	LLVMContext &CTX = F->getContext();
	Module *M = CI.getParent()->getParent()->getParent();

	const Type *intPtrType;
	if (M->getPointerSize() == Module::Pointer64)
	    intPtrType = Type::getInt64Ty(CTX);
	else if (M->getPointerSize() == Module::Pointer32)
	    intPtrType = Type::getInt32Ty(CTX);
	else
	    assert(0 && "Could not determine target pointer size");

	// replace these with the runtime implementation
	if (isa<SKIRPushInst>(&CI)) {
	    static Constant *pushFCache;
	    //Value *me = ConstantInt::get(intPtrType, (uintptr_t)rt);
	    Value *ops[2] = { CI.getOperand(1),     /* stream */
			      CI.getOperand(2) };   /* element */
	    /* void __SKIRRT_push(skir_stream_idx_t p, skir_stream_element_t e) */
	    ReplaceCallWith("__SKIR_push", &CI, ops, ops+2, Type::getVoidTy(CTX), pushFCache);
	    ret = true;
	}
	else if (isa<SKIRPopInst>(&CI)) {
	    static Constant *popFCache;
	    //Value *me = ConstantInt::get(intPtrType, (uintptr_t)rt);
	    Value *ops[2] = { CI.getOperand(1),     /* stream */
			      CI.getOperand(2) };   /* element */
	    /* void __SKIRRT_pop(skir_stream_idx_t p, skir_stream_element_t e) */
	    ReplaceCallWith("__SKIR_pop", &CI, ops, ops+2, Type::getVoidTy(CTX), popFCache);
	    ret = true;
	}
    }
};
char SKIRLowerStreamOpsPass::ID = 0;
RegisterPass<SKIRLowerStreamOpsPass> Y("skir-lower-stream-ops",
				       "SKIR Runtime LowerStreamOps", false, false);

}

namespace llvm {

ModulePass *createSKIRLowerGraphOpsPass()
{
    return new SKIRLowerGraphOpsPass();
}

FunctionPass *createSKIRLowerStreamOpsPass()
{
    return new SKIRLowerStreamOpsPass();
}

}

void SKIRRuntime::runSKIRLoweringPasses(SKIRRuntime *p, Module *m)
{
    PassManager PM;
    PM.add(new SKIRLowerGraphOpsPass((void*)p));
    //PM.add(new SKIRLowerStreamOpsPass((void *)p);
    PM.add(createVerifierPass());
    PM.run(*m);
}
