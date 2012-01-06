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

#ifndef SKIR_BUILDER_H
#define SKIR_BUILDER_H

#include <llvm/Module.h>
#include "llvm/Support/IRBuilder.h"
#include <llvm/Intrinsics.h>

#include "SKIRTypes.h"

namespace llvm {

class SKIRBuilder 
    : public IRBuilder<>
{
    LLVMContext &C;

public:
    SKIRBuilder(LLVMContext &C) : IRBuilder<>(C), C(C) {}
    explicit SKIRBuilder(BasicBlock *TheBB) 
	: IRBuilder<>(TheBB), C(TheBB->getContext()) {}
    SKIRBuilder(BasicBlock *TheBB, BasicBlock::iterator IP) 
	: IRBuilder<>(TheBB, IP), C(TheBB->getContext()) {}

    enum {
	SKIR_WORK,
	SKIR_INS,
	SKIR_OUTS,
	SKIR_STATE
    };

    Value *CreateStream(Module *mod);

    Value *CreateKernel(Module *mod, Value *work, Value *params);
    Value *CreateKernel(Module *mod, Value *init, Value *work, Value *params);

    Value *CreateKernelCall(Module *mod, Value *kernel, Value *ins, Value *outs);
    
    Value *CreateWait(Module *mod, Value *kernel);

    Value *CreatePush(Module *mod, Value *stream, Value *elm);
    
    Value *CreatePop(Module *mod, Value *stream, Value *elm_ptr);

    Value *CreatePeek(Module *mod, Value *stream, Value *elm_ptr, Value *offset);

    Value *CreateMalloc(const Type *AllocTy, Value *ArraySize, const Twine &NameStr);
};

}

#endif
