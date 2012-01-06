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

#include <SKIR/SKIRBuilder.h>

#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Intrinsics.h>
#include <llvm/Target/TargetData.h>
#include <llvm/ADT/Twine.h>

///
/// Helper functions for building SKIR.  Used primarily by the StreamIt frontend.
///

using namespace llvm;

Value *
SKIRBuilder::CreateStream(Module *mod) {
    Value *int_skir_stream = Intrinsic::getDeclaration(mod, Intrinsic::skir_stream);
    return CreateCall(int_skir_stream);
}

Value *
SKIRBuilder::CreateKernel(Module *mod, Value *init, Value *work, Value *params) {
    if (params == NULL)
	params = CreateIntToPtr(ConstantInt::get(Type::getInt64Ty(C), 0),GetVoidPtrType(C));
    params = CreateCall(init, params);
    return CreateKernel(mod, work, params);
}

Value *
SKIRBuilder::CreateKernel(Module *mod, Value *work, Value *params) {
    Value *int_skir_kernel = Intrinsic::getDeclaration(mod, Intrinsic::skir_kernel);
    Value *work_ptr = CreateBitCast(work, GetVoidPtrType(C));
    LLVMContext &C = mod->getContext();
    if (params == NULL)
	params = CreateIntToPtr(ConstantInt::get(Type::getInt64Ty(C), 0),GetVoidPtrType(C));
    return CreateCall2(int_skir_kernel, work_ptr, params);
}

Value *
SKIRBuilder::CreateKernelCall(Module *mod, Value *kernel, Value *ins, Value *outs) {
    LLVMContext &C = mod->getContext();
    Value *ptr = CreateBitCast(kernel, GetVoidPtrType(C));
    Value *int_skir_call = Intrinsic::getDeclaration(mod, Intrinsic::skir_call);
    if (ins == NULL)
	ins = CreateIntToPtr(ConstantInt::get(Type::getInt64Ty(C), 0), GetInsOutsType(C));
    else
	ins = CreateBitCast(ins, GetInsOutsType(C));
    if (outs == NULL)
	outs = CreateIntToPtr(ConstantInt::get(Type::getInt64Ty(C), 0), GetInsOutsType(C));
    else
	outs = CreateBitCast(outs, GetInsOutsType(C));
    return CreateCall3(int_skir_call, ptr, ins, outs);
}

Value *
SKIRBuilder::CreatePush(Module *mod, Value *stream, Value *elm)
{
    Value *int_skir_push = Intrinsic::getDeclaration(mod, Intrinsic::skir_push);
    elm = CreateBitCast(elm, GetStreamElementType(mod->getContext()));
    return CreateCall2(int_skir_push, stream, elm);
}

Value *
SKIRBuilder::CreatePop(Module *mod, Value *stream, Value *elm)
{
    Value *int_skir_pop = Intrinsic::getDeclaration(mod, Intrinsic::skir_pop);
    elm = CreateBitCast(elm, GetStreamElementType(mod->getContext()));
    CallInst *call = CreateCall2(int_skir_pop, stream, elm);
    call->setAttributes(Intrinsic::getAttributes(Intrinsic::skir_pop));
    return call;
}

Value *
SKIRBuilder::CreatePeek(Module *mod, Value *stream, Value *elm, Value *offset)
{
    Value *int_skir_peek = Intrinsic::getDeclaration(mod, Intrinsic::skir_peek);
    elm = CreateBitCast(elm, GetStreamElementType(mod->getContext()));
    unsigned nbits = offset->getType()->getScalarSizeInBits();
    if (nbits > Type::getInt32Ty(mod->getContext())->getScalarSizeInBits())
	offset = CreateTrunc(offset, Type::getInt32Ty(mod->getContext()));
    else if (nbits < Type::getInt32Ty(mod->getContext())->getScalarSizeInBits())
	offset = CreateSExt(offset, Type::getInt32Ty(mod->getContext()));
    return CreateCall3(int_skir_peek, stream, elm, offset);
}

Value *
SKIRBuilder::CreateWait(Module *mod, Value *kernel)
{
    Value *int_skir_wait = Intrinsic::getDeclaration(mod, Intrinsic::skir_wait);
    return CreateCall(int_skir_wait, kernel);
}

Value *
SKIRBuilder::CreateMalloc(const Type *allocTy, Value *arraySize, const Twine &nameStr)
{
    BasicBlock *BB = GetInsertBlock();
    Module *mod = BB->getParent()->getParent();

    TargetData TD(mod);
    const Type *intTy = TD.getIntPtrType(mod->getContext());
    unsigned size = TD.getTypeAllocSize(allocTy);
    Value *allocSize = ConstantInt::get(intTy, size);
    Instruction *result = CallInst::CreateMalloc(BB, intTy, allocTy, allocSize, arraySize, 
						 NULL, nameStr);
    BB->getInstList().push_back(result);
    return result;
}
