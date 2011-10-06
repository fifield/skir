#ifndef SKIR_TYPES_H
#define SKIR_TYPES_H

#include "llvm/DerivedTypes.h"

namespace llvm {

inline Type *GetVoidPtrType(LLVMContext &C) {
    return PointerType::get(Type::getInt8Ty(C),0);
}

inline Type *GetVoidPtrPtrType(LLVMContext &C) {
    return PointerType::get(GetVoidPtrType(C),0);
}

inline Type *GetStreamElementType(LLVMContext &C) {
    return GetVoidPtrType(C);
}

inline Type *GetInsOutsType(LLVMContext &C) {
    return GetVoidPtrPtrType(C);
}

inline FunctionType *GetWorkFunctionType(LLVMContext &C) {
    std::vector<const Type *> workParamsType;
    workParamsType.push_back(GetVoidPtrType(C)); // state
    workParamsType.push_back(GetInsOutsType(C)); // ins
    workParamsType.push_back(GetInsOutsType(C)); // outs
    return FunctionType::get(Type::getInt32Ty(C), workParamsType, false);
}

inline FunctionType *GetInitFunctionType(LLVMContext &C) {
    std::vector<const Type *> initParamsType;
    initParamsType.push_back(GetVoidPtrType(C));
    return FunctionType::get(GetVoidPtrType(C), initParamsType, false);
} 

}

#endif
