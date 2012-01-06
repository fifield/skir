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
