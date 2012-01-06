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

#ifndef _CREATEZEROVISITOR_HPP_
#define _CREATEZEROVISITOR_HPP_

#include "nodes/TypeVisitor.hpp"
#include "nodes/ExprConstInt.hpp"
#include "ExprEvalVisitor.hpp"

//#define lType llvm::Type
//#define sType streamit::Type
//#define lFunction llvm::Function
//#define sFunction streamit::Function

// create a expression representing zero for a given type

namespace streamit {

class CreateZeroVisitor : public TypeVisitor
{
private:
    //    llvm::LLVMContext &CTX;
    FEContext *ctx;
    
public:
    //TypeConvert(llvm::LLVMContext &C) : CTX(C) {}
    CreateZeroVisitor(FEContext *context): ctx(context) {}
    
    Expression *getZeroExprOfType(Type *ty) {
	return (Expression*)ty->accept(this);
    }

    void *visitType(Type *t) { return NULL; }

    void *visitTypeArray(TypeArray *t) {
	ExprEvalVisitor ev;
	size_t length = (size_t)t->getLength()->accept(&ev);

	ExpressionList *el = new ExpressionList;
	for (size_t i=0; i<length; i++)
	    el->push_back((Expression*)t->getBase()->accept(this));
	
	return (new ExprArrayInit(ctx, el));
    }

    void *visitTypeHelper(TypeHelper *t) { return NULL; }

    void *visitTypePrimitive(TypePrimitive *t) {
	switch (t->getType()) {

	case TypePrimitive::TYPE_INT:
	    return (Expression*)(new ExprConstInt(ctx, 0));

	case TypePrimitive::TYPE_FLOAT:
	case TypePrimitive::TYPE_DOUBLE:
	    return (Expression*)(new ExprConstFloat(ctx, 0.0));

	case TypePrimitive::TYPE_BOOLEAN:
	    return (Expression*)(new ExprConstBoolean(ctx, false));

	case TypePrimitive::TYPE_CHAR:
	    return (Expression*)(new ExprConstChar(ctx, 0));

	case TypePrimitive::TYPE_COMPLEX:
	    return (Expression*)(new ExprComplex(ctx, 
						 (Expression*)(new ExprConstFloat(ctx, 0.0)),
						 (Expression*)(new ExprConstFloat(ctx, 0.0))));

	case TypePrimitive::TYPE_STRING:
	case TypePrimitive::TYPE_BIT:
	    assert(0 && "unimplemented");
	    break;

	default:
	    return NULL;
	}
	return NULL;
    }

    void *visitTypeStructRef(TypeStructRef *t) { 
	assert(0);
	return NULL;
    }
    void *visitTypeStruct(TypeStruct *t) {
	assert(0);
	return NULL;
    }
};

}

#endif
