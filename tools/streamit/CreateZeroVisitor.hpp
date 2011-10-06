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
