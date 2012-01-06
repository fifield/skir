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

#ifndef _TYPECONVERT_HPP_
#define _TYPECONVERT_HPP_

#include "nodes/TypeVisitor.hpp"
#include "ExprEvalVisitor.hpp"

#include <llvm/LLVMContext.h>
#include <llvm/DerivedTypes.h>

#define lType llvm::Type
#define sType streamit::Type
#define lFunction llvm::Function
#define sFunction streamit::Function

namespace streamit {

class TypeConvert : public TypeVisitor
{
private:
    llvm::LLVMContext &CTX;
    
public:
    TypeConvert(llvm::LLVMContext &C) : CTX(C) {}
    
    lType *convert(sType *t) {
	return (lType*)t->accept(this);
    }

    lType *convert(sFunction *t) {
	assert(0);
	return (lType*)0;
    }

    void *visitType(Type *t) { return NULL; }

    void *visitTypeArray(TypeArray *t) {
	ExprEvalVisitor ev;
	lType *baseType = (lType*)t->getBase()->accept(this);
	size_t length = (size_t)t->getLength()->accept(&ev);
	return llvm::ArrayType::get(baseType, length);
    }

    void *visitTypeHelper(TypeHelper *t) { return NULL; }

    void *visitTypePrimitive(TypePrimitive *t) {
	const lType *ret = NULL;
	switch (t->getType()) {
	case TypePrimitive::TYPE_INT:
	    ret = lType::getInt32Ty(CTX);
	    break;
	case TypePrimitive::TYPE_FLOAT:
	    ret = lType::getFloatTy(CTX);
	    break;
	case TypePrimitive::TYPE_DOUBLE:
	    ret = lType::getDoubleTy(CTX);
	    break;
	case TypePrimitive::TYPE_VOID:
	    ret = lType::getVoidTy(CTX);
	    break;
	case TypePrimitive::TYPE_BOOLEAN:
	case TypePrimitive::TYPE_BIT:
	    ret = lType::getInt1Ty(CTX);
	    break;
	case TypePrimitive::TYPE_STRING:
	    ret = llvm::PointerType::get(lType::getInt8Ty(CTX),0);
	    break;
	case TypePrimitive::TYPE_INTPTR:
	    ret = llvm::PointerType::get(lType::getInt32Ty(CTX),0);
	    break;
	case TypePrimitive::TYPE_CHAR:
	    ret = lType::getInt8Ty(CTX);
	    break;

	// todo
	case TypePrimitive::TYPE_COMPLEX:
	default:
	    break;
	}
	return (void *)ret;
    }

    void *visitTypeStructRef(TypeStructRef *t) { return NULL; }
    void *visitTypeStruct(TypeStruct *t) { return NULL; }
};

}

#endif
