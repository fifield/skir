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
	SKIR_INIT,
	SKIR_WORK,
	SKIR_INS,
	SKIR_OUTS,
	SKIR_STATE
    };

    Value *CreateStream(Module *mod);

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
