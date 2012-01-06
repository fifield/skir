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

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Instruction.h>
#include <llvm/InstrTypes.h>

#include "SKIR/SKIRBuilder.h"
#include "SKIR/SKIRTypes.h"
#include "FEIRToSKIR.hpp"
#include "nodes/IsZeroVisitor.hpp"
//#include "nodes/IsConstIntVisitor.hpp"
//#include "CreateZeroVisitor.hpp"

#include <string>
#include <math.h>

using namespace std;
using namespace llvm;
using namespace streamit;

// deal with some collisions
#define lType llvm::Type
#define sType streamit::Type
#define lFunction llvm::Function
#define sFunction streamit::Function

// used with the splitjoin private member
// when processing splitjoins
#define split true
#define join false


static Instruction *
getAllocaPoint(lFunction *F)
{
    static lFunction *savedF = 0;
    static Instruction *allocaPoint = 0;
    if (savedF == F)
	return allocaPoint;
    savedF = F;

    BasicBlock &entry = F->getEntryBlock();
    BasicBlock::iterator BI = entry.begin(), BE = entry.end();
    allocaPoint = BI;
    while (BI != BE) {
	Instruction *inst = BI++;
	if (isa<BitCastInst>(inst))
	    if (inst->getName() == "alloca point") {
		allocaPoint = inst;
		return allocaPoint;
	    }
    }
    return allocaPoint;
}

void *FEIRToSKIR::visitExprArray(ExprArray *exp) {
    debug("in visitExprArray");

    bool b = inLHS;
    inLHS = true;
    Value *base = (Value *)exp->getBase()->accept(this);
    inLHS = false;
    Value *offset = (Value *)exp->getOffset()->accept(this);
    inLHS = b;
    Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
    Value *idx[2] = {zero, offset};

    SKIRBuilder builder(currentBB);
    Value *ptr = builder.CreateGEP(base, idx, idx+2);
    if (inLHS)
	return ptr;
    else
	return builder.CreateLoad(ptr);
}

void *FEIRToSKIR::visitExprArrayInit(ExprArrayInit *exp) {
    error("in visitArrayInit");
    return NULL;
}

void FEIRToSKIR::doArrayInit(Value *arrayPtr, ExprArrayInit *init)
{
    SKIRBuilder builder(currentBB);
    ExpressionList *el = init->getElements();
    ExpressionList::iterator iter;
    int idx = 0;

    if (init->getDims() > 1) {
	for (iter = el->begin(); iter != el->end(); iter++) {
	    Expression *e = *iter;
	    Value *p = builder.CreateStructGEP(arrayPtr, idx++);
	    doArrayInit(p, (ExprArrayInit*)e);
	}
    } else {
	for (iter = el->begin(); iter != el->end(); iter++) {
	    Expression *e = *iter;
	    Value *p = builder.CreateStructGEP(arrayPtr, idx++);
	    Value *v = (Value *)e->accept(this);
	    v = matchTypes(v, cast<PointerType>(p->getType())->getElementType());
	    builder.CreateStore(v, p);
	}
    }
}

void *FEIRToSKIR::visitExprBinary(ExprBinary *exp) { 
    debug("in visitExprBinary");

    Expression *l = exp->getLeft();
    Expression *r = exp->getRight();
    int op = exp->getOp();

    Value *vl = (Value *)l->accept(this);
    Value *vr = (Value *)r->accept(this);

    const lType *vlty = vl->getType();
    const lType *vrty = vr->getType();

    if (!vlty->isFloatingPointTy() && vrty->isFloatingPointTy())
	vl = matchTypes(vl, vrty);
    else if (vlty->isFloatingPointTy() && !vrty->isFloatingPointTy())
    	vr = matchTypes(vr, vlty);

    SKIRBuilder builder(currentBB);

    switch (op) {

    case ExprBinary::BINOP_ADD:
	return builder.CreateAdd(vl, vr);

    case ExprBinary::BINOP_SUB:
	return builder.CreateSub(vl, vr);

    case ExprBinary::BINOP_MUL:
	return builder.CreateMul(vl, vr);

    case ExprBinary::BINOP_DIV:
	if (vl->getType()->isFPOrFPVectorTy())
	    return builder.CreateFDiv(vl, vr);
	else
	    return builder.CreateSDiv(vl, vr);	    

    case ExprBinary::BINOP_MOD:
	if (vl->getType()->isFPOrFPVectorTy())
	    return builder.CreateFRem(vl, vr);
	else
	    return builder.CreateSRem(vl, vr);	    

    case ExprBinary::BINOP_AND: {
	const lType *t;
	t = vl->getType();
	if (t != lType::getInt1Ty(CTX)) {
	    if (t->isIntegerTy())
		vl = builder.CreateICmpNE(vl, ConstantInt::get(t, 0));
	    else if (t->isFloatingPointTy())
		vl = builder.CreateFCmpONE(vl, ConstantFP::get(t, 0));
	t = vr->getType();
	}
	if (t != lType::getInt1Ty(CTX)) {
	    if (t->isIntegerTy())
		vr = builder.CreateICmpNE(vr, ConstantInt::get(t, 0));
	    else if (t->isFloatingPointTy())
		vr = builder.CreateFCmpONE(vr, ConstantFP::get(t, 0));
	}
	return builder.CreateAnd(vl, vr);
    }

    case ExprBinary::BINOP_OR: {
	const lType *t;
	t = vl->getType();
	if (t != lType::getInt1Ty(CTX)) {
	    if (t->isIntegerTy())
		vl = builder.CreateICmpNE(vl, ConstantInt::get(t, 0));
	    else if (t->isFloatingPointTy())
		vl = builder.CreateFCmpONE(vl, ConstantFP::get(t, 0));
	}
	t = vr->getType();
	if (t != lType::getInt1Ty(CTX)) {
	    if (t->isIntegerTy())
		vr = builder.CreateICmpNE(vr, ConstantInt::get(t, 0));
	    else if (t->isFloatingPointTy())
		vr = builder.CreateFCmpONE(vr, ConstantFP::get(t, 0));
	}
	return builder.CreateOr(vl, vr);
    }

    case ExprBinary::BINOP_EQ:
	if (vl->getType()->isFloatingPointTy())
	    return builder.CreateFCmpOEQ(vl, vr);	     
	else
	    return builder.CreateICmpEQ(vl, vr);

    case ExprBinary::BINOP_NEQ:
	if (vl->getType()->isFloatingPointTy())
	    return builder.CreateFCmpONE(vl, vr);	     
	else
	    return builder.CreateICmpNE(vl, vr);

    case ExprBinary::BINOP_LT:
	if (vl->getType()->isFloatingPointTy())
	    return builder.CreateFCmpOLT(vl, vr);	     
	else
	    return builder.CreateICmpSLT(vl, vr);
	
    case ExprBinary::BINOP_LE:
	if (vl->getType()->isFloatingPointTy())
	    return builder.CreateFCmpOLE(vl, vr);	     
	else
	    return builder.CreateICmpSLE(vl, vr);
	
    case ExprBinary::BINOP_GT:
	if (vl->getType()->isFloatingPointTy())
	    return builder.CreateFCmpOGT(vl, vr);	     
	else
	    return builder.CreateICmpSGT(vl, vr);

    case ExprBinary::BINOP_GE:
	if (vl->getType()->isFloatingPointTy())
	    return builder.CreateFCmpOGE(vl, vr);	     
	else
	    return builder.CreateICmpSGE(vl, vr);

    case ExprBinary::BINOP_BAND:
	return builder.CreateAnd(vl, vr);

    case ExprBinary::BINOP_BOR:
	return builder.CreateOr(vl, vr);

    case ExprBinary::BINOP_BXOR:
	return builder.CreateXor(vl, vr);

    case ExprBinary::BINOP_LSHIFT:
	return builder.CreateShl(vl, vr);

    case ExprBinary::BINOP_RSHIFT:
	//return builder.CreateLShr(vl, vr);
	return builder.CreateAShr(vl, vr);

    default:
	assert(0 && "Bad ExprBinary operation!");
	break;
    }
    return NULL;
}

//void *FEIRToSKIR::visitExprComplex(ExprComplex *exp) { return NULL; }
//void *FEIRToSKIR::visitExprComposite(ExprComposite *exp) { return NULL; }

void *FEIRToSKIR::visitExprConstBoolean(ExprConstBoolean *exp) {
    debug("in visitExprConstBoolean");

    bool b = exp->getVal();
    return ConstantInt::get(lType::getInt1Ty(CTX), (int)b);
}

void *FEIRToSKIR::visitExprConstChar(ExprConstChar *exp) {
    debug("in visitExprConstChar");

    char c = exp->getVal();
    return ConstantInt::get(lType::getInt8Ty(CTX), c);

}

void *FEIRToSKIR::visitExprConstFloat(ExprConstFloat *exp) {
    debug("in visitExprConstFloat");

    float d = exp->getVal();
    return ConstantFP::get(lType::getFloatTy(CTX), d);
}

void *FEIRToSKIR::visitExprConstInt(ExprConstInt *exp) {
    debug("in visitExprConstInt");

    int i = exp->getVal();
    return ConstantInt::get(lType::getInt32Ty(CTX), i);
}

void *FEIRToSKIR::visitExprConstStr(ExprConstStr *exp) {
    debug("in visitExprConstStr");

    string s = exp->getVal();
    SKIRBuilder builder(currentBB);
    return builder.CreateGlobalStringPtr(s.c_str());
}

//void *FEIRToSKIR::visitExprDynamicToken(ExprDynamicToken *exp) { return NULL; }
//void *FEIRToSKIR::visitExprField(ExprField *exp) { return NULL; }

void *FEIRToSKIR::visitExprFunCall(ExprFunCall *exp)
{
    debug("in visitExprFunCall");

    // lookup this name in the symbol table
    // if a function node is not found, bail...
    string callName = exp->getName();
    FENode *node = syms.getNode(callName);
    if (!node || !node->isFunction())
	error("call to undefined function '"+callName+"'", exp);
    sFunction *sfunc = (sFunction *)node;
    string funcName = sfunc->getName();

    // get the arguments
    ExpressionList *el = exp->getParams();
    ExpressionList::iterator eiter;
    vector<Value*> args;
    for (eiter = el->begin(); eiter != el->end(); eiter++) {
	Expression *e = *eiter;
	Value *arg = (Value*)e->accept(this);
	args.push_back(arg);
    }

    // get and check(XXX TODO) the parameter types
    vector<const lType*> paramTypes;
    ParameterList *pl = sfunc->getParams();
    ParameterList::iterator piter;
    for (piter = pl->begin(); piter != pl->end(); piter++) {
	Parameter *p = *piter;
	paramTypes.push_back(tConvert.convert(p->getType()));
    }

    const lType *retType = tConvert.convert(sfunc->getReturnType());
    FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
    Value *func = getOrInsertFunction(funcName, funcType);

    SKIRBuilder builder(currentBB);
    return builder.CreateCall(func, args.begin(), args.end());
}

//void *FEIRToSKIR::visitExprHelperCall(ExprHelperCall *exp) { return NULL; }

void *FEIRToSKIR::visitExprPeek(ExprPeek *exp) {
    debug("in visitExprPeek");

    SKIRBuilder builder(currentBB);

    Value *stream = ConstantInt::get(lType::getInt32Ty(CTX),0);
    lType *ety = tConvert.convert(currentStreamSpec->getStreamType()->getIn());
    //Value *e = builder.CreateAlloca(ety);
    Value *e = new AllocaInst(ety, "", getAllocaPoint(currentBB->getParent()));
    Value *n = (Value *)exp->getExpr()->accept(this);
    const lType *eptrty = e->getType();

    e = builder.CreateBitCast(e, GetVoidPtrType(CTX));
    builder.CreatePeek(mod, stream, e, n);
    e = builder.CreateBitCast(e, eptrty);
    e = builder.CreateLoad(e);

    return e;
}

void *FEIRToSKIR::visitExprPop(ExprPop *exp) {
    debug("in visitExprPop");

    SKIRBuilder builder(currentBB);

    Value *stream = ConstantInt::get(lType::getInt32Ty(CTX),0);
    lType *ety = tConvert.convert(currentStreamSpec->getStreamType()->getIn());
    //Value *e = builder.CreateAlloca(ety);
    Value *e = new AllocaInst(ety, "", getAllocaPoint(currentBB->getParent()));
    const lType *eptrty = e->getType();

    e = builder.CreateBitCast(e, GetVoidPtrType(CTX));
    builder.CreatePop(mod, stream, e);
    e = builder.CreateBitCast(e, eptrty);
    e = builder.CreateLoad(e);

    return e;
}

//void *FEIRToSKIR::visitExprRange(ExprRange *exp) { return NULL; }

void *FEIRToSKIR::visitExprTernary(ExprTernary *exp) {
    debug("in visitExprTernary");

    if (exp->getOp() != ExprTernary::TEROP_COND)
	error("bad ternary operation", exp);
    
    SKIRBuilder builder(currentBB);

    // evaluate cond to true/false
    Value *cond = (Value *)exp->getA()->accept(this);
    // handle cases like if (i)
    const lType *t = cond->getType();
    if (t != lType::getInt1Ty(CTX)) {
	if (t->isIntegerTy())
	    cond = builder.CreateICmpNE(cond, ConstantInt::get(t, 0));
	else if (t->isFloatingPointTy())
	    cond = builder.CreateFCmpONE(cond, ConstantFP::get(t, 0));
    }

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    lFunction *currentFunction = builder.GetInsertBlock()->getParent();
    BasicBlock *thenBB = BasicBlock::Create(CTX,"then", currentFunction);
    BasicBlock *elseBB = BasicBlock::Create(CTX,"else");
    BasicBlock *mergeBB = BasicBlock::Create(CTX,"ifcont");
    
    Expression *cons = exp->getB();
    Expression *alt = exp->getC();

    builder.CreateCondBr(cond, cons ? thenBB : mergeBB, alt ? elseBB : mergeBB);

    // codegen then block
    Value *thenV;
    if (cons) {
	currentBB = thenBB;
	thenV = (Value *)cons->accept(this);
	thenBB = currentBB;
	builder.SetInsertPoint(thenBB);
	builder.CreateBr(mergeBB);
    }

    // codegen else block
    Value *elseV;
    if (alt) {
	currentFunction->getBasicBlockList().push_back(elseBB);
	currentBB = elseBB;
	elseV = (Value *)alt->accept(this);
	elseBB = currentBB;
	builder.SetInsertPoint(elseBB);
	builder.CreateBr(mergeBB);
    }

    t = elseV->getType();
    if (t != thenV->getType())
	error("branches of conditional ExprTernary have different types!",exp);
    
    // attach merge block
    currentBB = mergeBB;
    currentFunction->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(currentBB);
    PHINode *phi = builder.CreatePHI(t);
    phi->addIncoming(thenV, thenBB);
    phi->addIncoming(elseV, elseBB);

    return phi;
}

void *FEIRToSKIR::visitExprTypeCast(ExprTypeCast *exp)
{ 
    const lType *ltype = tConvert.convert(exp->getType());
    return matchTypes((Value *)exp->getExpr()->accept(this), ltype);
}

void *FEIRToSKIR::visitExprUnary(ExprUnary *exp) {
    debug("in visitExprUnary");

    Expression *e = exp->getExpr();
    Expression *one = new ExprConstInt(1);

    Value *v = (Value *)e->accept(this);

    int op = exp->getOp();
    StmtAssign *stmt;
    switch (op) {

    case ExprUnary::UNOP_PREINC:
	stmt = new StmtAssign(e->getContext(), e, one, ExprBinary::BINOP_ADD);
	return stmt->accept(this);

    case ExprUnary::UNOP_POSTINC:
	stmt = new StmtAssign(e->getContext(), e, one, ExprBinary::BINOP_ADD);
	stmt->accept(this);
	return v;

    case ExprUnary::UNOP_PREDEC:
	stmt = new StmtAssign(e->getContext(), e, one, ExprBinary::BINOP_SUB);
	return stmt->accept(this);

    case ExprUnary::UNOP_POSTDEC:
	stmt = new StmtAssign(e->getContext(), e, one, ExprBinary::BINOP_SUB);
	stmt->accept(this);
	return v;

    case ExprUnary::UNOP_COMPLEMENT: {
	SKIRBuilder builder(currentBB);
 	return builder.CreateNot(v);
    }

    case ExprUnary::UNOP_NOT: {
	SKIRBuilder builder(currentBB);
	const lType *t = v->getType();
	if (t != lType::getInt1Ty(CTX)) {
	    if (t->isIntegerTy())
		v = builder.CreateICmpNE(v, ConstantInt::get(t, 0));
	    else if (t->isFloatingPointTy())
		v = builder.CreateFCmpONE(v, ConstantFP::get(t, 0));
	}
	v = builder.CreateNot(v);
    }

    case ExprUnary::UNOP_NEG: {
	SKIRBuilder builder(currentBB);
	return builder.CreateNeg(v);
    }

    default:
	assert(0 && "Bad ExprUnary operation!");
	break;
    }
    return NULL;
}

void *FEIRToSKIR::visitExprVar(ExprVar *exp) {
    debug("in visitExprVar");
    string name = exp->getName();
    Value *v = syms.getValue(name);

    if (!v) error("undefined ExprVar: "+name);

    // if this thing is an integer, then it's an offset into state
    // all other values in the symbol table are pointers
    if (v->getType()->isIntegerTy()) {
	assert(currentStatePtr);
	SKIRBuilder builder(currentBB);
	Value *idxs[2] = {ConstantInt::get(lType::getInt32Ty(CTX), 0), v};
	v = builder.CreateGEP(currentStatePtr, idxs, idxs+2);
	//v = builder.CreateLoad(v);
    }

    // if it's a RHS, load and return the value of the var
    if (!inLHS) {
	SKIRBuilder builder(currentBB);
	v = builder.CreateLoad(v, name.c_str());
    }

    return v;
}

void *FEIRToSKIR::visitFieldDecl(FieldDecl *field)
{
    debug("in visitFieldDecl");
    assert(0 && "shouldn't be in visitField Decl");
    return NULL;
}

void *FEIRToSKIR::visitFunction(sFunction *func) {
    debug("in visitFunction");

    string name;
    const FunctionType *type;
    switch (func->getCls()) {
    case sFunction::FUNC_INIT:
    case sFunction::FUNC_WORK:
	warn("warning: INIT or WORK function in visitFunction!", func);
	return NULL;
	break;
    default:
	name = func->getName();
	// XXX this should be the actual type of func 
	// inittype is void* f(void*)
	type = GetInitFunctionType(CTX);
	break;
    }

    Constant *c = mod->getOrInsertFunction(name, type);
    lFunction *f = cast<lFunction>(c);
    currentBB = BasicBlock::Create(CTX,"entry", f);
    Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
    new BitCastInst(zero, zero->getType(), "alloca point", currentBB);

    Statement *body = func->getBody();
    syms.pushScope(name);
    body->accept(this);
    syms.popScope();

    SKIRBuilder builder(currentBB);
    builder.CreateRetVoid();
    currentBB = NULL;
    return NULL;
}

// not part of the visitor interface
// func might be NULL, but we still need to handle state and stream parameters
void *FEIRToSKIR::visitFuncInit(Function *func) {
    debug("in visitFuncInit");

    string name = "__streamit_" + currentStreamSpec->getName() + "_init";
    int streamType = currentStreamSpec->getType();

    // create the init function
    Constant *c = getOrInsertFunction(name, GetInitFunctionType(CTX));
    lFunction *f = cast<lFunction>(c);
    syms.addEntry(syms.getScope() + "::init", func, f, true);
    currentBB = BasicBlock::Create(CTX,"entry", f);
    Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
    new BitCastInst(zero, zero->getType(), "alloca point", currentBB);
    SKIRBuilder builder(currentBB);

    //
    //  Build kernel state type information
    //

    // list of types of init function parameters
    vector<const lType *> paramTypes;
    // list of types of init function parameters + kernel state variables
    vector<const lType *> fieldTypes;

    // record parameter types
    ParameterList *pl = currentStreamSpec->getParams();
    ParameterList::iterator piter;
    for (piter = pl->begin(); piter != pl->end(); piter++) {
	Parameter *p = *piter;
	const lType *fieldType = tConvert.convert(p->getType());
	fieldTypes.push_back(fieldType);
	paramTypes.push_back(fieldType);
    }

    // argType = llvm type of the argument to the init function
    lType *argType = StructType::get(CTX,paramTypes);

    // record fielddecl types
    FieldDeclList *fdl = currentStreamSpec->getFields();
    FieldDeclList::iterator diter;
    for (diter = fdl->begin(); diter != fdl->end(); diter++) {
	FieldDecl *fd = *diter;
	for (int i=0; i<fd->getNumFields(); i++) {
	    // frontend type -> llvm type
	    const lType *fieldType = tConvert.convert(fd->getType(i));
	    fieldTypes.push_back(fieldType);
	}
    }
    
    // 
    // Allocate kernel state
    // 

    Value *one = ConstantInt::get(lType::getInt64Ty(CTX), 1);
    lType *stateType = StructType::get(CTX,fieldTypes);
    currentStatePtr = builder.CreateMalloc(stateType, one, "state");
    builder.CreateStore(Constant::getNullValue(stateType), currentStatePtr);    

    //
    // Init kernel state
    //
    
    int currentIdx = 0; // current offset into state

    // copy filter parameters to the kernel state
    Value *arg = builder.CreateBitCast(f->arg_begin(), PointerType::get(argType,0), "args");
    int argIdx = 0;
    for (piter = pl->begin(); piter != pl->end(); piter++) {
	Parameter *p = *piter;

	// store offset into state in symbol table
	string fieldName = p->getName();
	Value *idx = ConstantInt::get(llvm::Type::getInt32Ty(CTX), currentIdx);
	syms.addEntry(fieldName, syms.getNode(fieldName), idx);

	// copy parameter into state
	Value *arg_ptr = builder.CreateStructGEP(arg, argIdx);
	Value *arg_val = builder.CreateLoad(arg_ptr);
	Value *state_ptr = builder.CreateStructGEP(currentStatePtr, currentIdx);
	builder.CreateStore(arg_val, state_ptr);

	currentIdx++;
	argIdx++;
    }

    // update the symbol table with the variable location,
    // initialize kernel state variables to zero (streamit requires this)
    for (diter = fdl->begin(); diter != fdl->end(); diter++) {
	FieldDecl *fd = *diter;
	for (int i=0; i<fd->getNumFields(); i++) {
	    Value *state_ptr = 0;

	    if (streamType == StreamSpec::STREAM_FILTER) {
		// store offset into state in symbol table
		string fieldName = fd->getName(i);
		Value *idx = ConstantInt::get(llvm::Type::getInt32Ty(CTX), currentIdx);
		syms.addEntry(fieldName, syms.getNode(fieldName), idx);
	    } 
	    else if (streamType == StreamSpec::STREAM_GLOBAL) {
		// allocate new global
		string fieldName = fd->getName(i);
		const lType *fieldType = tConvert.convert(fd->getType(i));
		state_ptr = new GlobalVariable(*mod, fieldType, false, 
					       GlobalVariable::InternalLinkage,
					       Constant::getNullValue(fieldType),
					       fieldName);
		syms.addEntry(fieldName, syms.getNode(fieldName), state_ptr);
	    }  
	    else {
		assert(0);
	    }

	    // initialize field
	    Expression *fdInit = fd->getInit(i);
	    if (streamType == StreamSpec::STREAM_FILTER)
		state_ptr = builder.CreateStructGEP(currentStatePtr, currentIdx);

	    if (fdInit) {
		if (fdInit->isArrayInit()) {
		    doArrayInit(state_ptr, (ExprArrayInit*)fdInit);
		} else {
		    Value *v = (Value *)fdInit->accept(this);
		    v = matchTypes(v, cast<PointerType>(state_ptr->getType())->getElementType());
		    builder.CreateStore(v, state_ptr);
		}
	    }
	    
	    currentIdx++;
	}
    }

    //
    // codegen init() body
    //
    if (func) {
	Statement *body = func->getBody();
	syms.pushScope("init");
	body->accept(this);
	syms.popScope();
    }

    if (streamType == StreamSpec::STREAM_FILTER) {
	// return state pointer
	builder.SetInsertPoint(currentBB);
	builder.CreateRet(builder.CreateBitCast(currentStatePtr, GetVoidPtrType(CTX)));
    }
    else if (streamType == StreamSpec::STREAM_GLOBAL) {
	// return null
	builder.CreateRet(ConstantPointerNull::get(cast<PointerType>(GetVoidPtrType(CTX))));
    }
    currentBB = NULL;
    return NULL;    
}

void FEIRToSKIR::createPipeline(Function *init, string worker)
{
    debug("in createPipeline");

    //
    // build init()
    //
    {
	string name = "__streamit_" + currentStreamSpec->getName() + "_init";
	Constant *c = getOrInsertFunction(name, GetInitFunctionType(CTX));
	lFunction *f = cast<lFunction>(c);
	syms.addEntry(syms.getScope() + "::init", init, f, true);
	currentBB = BasicBlock::Create(CTX,"entry", f);
	Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
	new BitCastInst(zero, zero->getType(), "alloca point", currentBB);
	SKIRBuilder builder(currentBB);

	vector<const lType *> stateTypes;
	vector<const lType *> paramTypes;
	int currentIdx = 0;

	// num_children
	stateTypes.push_back(lType::getInt32Ty(CTX));
	// pointer to num_children (for splits)
	stateTypes.push_back(PointerType::get(lType::getInt32Ty(CTX),0));
	// child array
	stateTypes.push_back(ArrayType::get(GetVoidPtrType(CTX), 512));
	
	// record programmer defined parameter types
	ParameterList *pl = currentStreamSpec->getParams();
	ParameterList::iterator piter;
	for (piter = pl->begin(); piter != pl->end(); ++piter) {
	    Parameter *p = *piter;
	    const lType *fieldType = tConvert.convert(p->getType());
	    stateTypes.push_back(fieldType);
	    paramTypes.push_back(fieldType);
	}
	// the type of the arguments parameter passed to the init function
	lType *argType = StructType::get(CTX,paramTypes);
	
	// allocate the state structure (num_children, children ptr, child array, parameters)
	Value *one = ConstantInt::get(lType::getInt64Ty(CTX), 1);
	lType *stateType = StructType::get(CTX,stateTypes);
	currentStatePtr = builder.CreateMalloc(stateType, one, "state");
	builder.CreateStore(Constant::getNullValue(stateType), currentStatePtr);

	syms.pushScope("init");

	// set num_children = 0
	Value *num_children_ptr = builder.CreateStructGEP(currentStatePtr, currentIdx);
	builder.CreateStore(ConstantInt::get(llvm::Type::getInt32Ty(CTX), 0), num_children_ptr);
	// store index of num_children in symbol table
	Value *num_children_idx = ConstantInt::get(llvm::Type::getInt32Ty(CTX), currentIdx);
	syms.addEntry(string("__num_children"), NULL, num_children_idx);
	currentIdx++;

	// store pointer to num_children
	Value *num_children_ptr_ptr = builder.CreateStructGEP(currentStatePtr, currentIdx);
	builder.CreateStore(num_children_ptr, num_children_ptr_ptr);

	// store index of num_children_ptr in symbol table
	Value *num_children_ptr_idx = ConstantInt::get(llvm::Type::getInt32Ty(CTX), currentIdx);
	syms.addEntry(string("__num_children_ptr"), NULL, num_children_ptr_idx);
	currentIdx++;

	// store index of children array in symbol table
	Value *children_idx = ConstantInt::get(llvm::Type::getInt32Ty(CTX), currentIdx);
	syms.addEntry(string("__children"), NULL, children_idx);
	currentIdx++;

	syms.popScope();

	// copy parameters to state
	Value *arg;
	int argIdx = 0;
	for (piter = pl->begin(); piter != pl->end(); ++piter) {
	    Parameter *p = *piter;

	    if (piter == pl->begin())
		arg = builder.CreateBitCast(f->arg_begin(), PointerType::get(argType,0), "args");
	    
	    // store offset into state in symbol table
	    string fieldName = p->getName();
	    Value *idx = ConstantInt::get(llvm::Type::getInt32Ty(CTX), currentIdx);
	    syms.addEntry(fieldName, syms.getNode(fieldName), idx);
	    
	    // copy parameter into state
	    Value *arg_ptr = builder.CreateStructGEP(arg, argIdx);
	    Value *arg_val = builder.CreateLoad(arg_ptr);
	    Value *state_ptr = builder.CreateStructGEP(currentStatePtr, currentIdx);
	    builder.CreateStore(arg_val, state_ptr);
	    
	    currentIdx++;
	    argIdx++;
	}

	syms.pushScope("init");
	init->getBody()->accept(this);
	syms.popScope();

	builder.SetInsertPoint(currentBB);

	//Value *num_children = builder.CreateLoad(num_children_ptr);
	//Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
	//Value *idx[2] = { zero, num_children };
	//Value *end = builder.CreateGEP(children_ptr, idx, idx+2);
	//zero = ConstantPointerNull::get(cast<PointerType>(GetVoidPtrType(CTX)));
	//builder.CreateStore( zero, end);
	
	builder.CreateRet( builder.CreateBitCast(currentStatePtr, GetVoidPtrType(CTX)) );

	lFunction *F = currentBB->getParent();
	Instruction *splitInst = 0;
	Instruction *joinInst = 0;
	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ) {
	    Instruction *inst = &*I; ++I;
	    if (!isa<SKIRKernelInst>(inst)) continue;

	    Value *calledFunction = 0;
	    if (isa<lFunction>(inst->getOperand(2))) {
		calledFunction = inst->getOperand(2);
	    } 
	    else {
		Instruction *cast = (Instruction*)(inst->getOperand(2));
		assert(cast);
		calledFunction = cast->getOperand(0);
	    }
	    assert(calledFunction);
	    assert(isa<lFunction>(calledFunction));
	    
	    const std::string name(calledFunction->getNameStr());
	    if (std::string::npos != name.find("split"))
		splitInst = inst;
	    else if (std::string::npos != name.find("join"))
		joinInst = inst;
	}

	// move the split to right before the join
	if (splitInst && joinInst) {
	    splitInst->moveBefore(joinInst);
	    Value::use_iterator UI, UE;
	    UI = splitInst->use_begin();
	    UE = splitInst->use_end();
	    for (; UI!= UE; ++UI) {
		Instruction *user = cast<Instruction>(*UI);
		assert(isa<StoreInst>(user));
		user->moveBefore(joinInst);
	    }
	}

    }
    //
    // work()
    //
    {
	Constant *c = getOrInsertFunction("__streamit_"+currentStreamSpec->getName()+"_work", 
					       GetWorkFunctionType(CTX));
	lFunction *f = cast<lFunction>(c);
	syms.addEntry(syms.getScope() + "::work", NULL, f, true);
	BasicBlock *bb = BasicBlock::Create(CTX,"entry", f);
	Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
	new BitCastInst(zero, zero->getType(), "alloca point", bb);
	SKIRBuilder builder(bb);
	
	// get arguments
	lFunction::arg_iterator args = f->arg_begin();
	Value *state = args++;
	Value *ins;
	if ( currentStreamSpec->getStreamType()->getIn()->isVoid() ) {
	    ins = ConstantPointerNull::get(cast<PointerType>(GetVoidPtrType(CTX)));
	    args++;
	}
	else
	    ins = args++;
	Value *outs = args;
	state = builder.CreateBitCast(state, currentStatePtr->getType(), "state");
	
	Value *init = syms.getValue("streamit::"+worker+"::init", true);
	Value *work = syms.getValue("streamit::"+worker+"::work", true);
	Value *kernel = builder.CreateKernel(mod, init, work, f->arg_begin());
	builder.CreateKernelCall(mod, kernel, ins, outs);
	builder.CreateRet(ConstantInt::get(lType::getInt32Ty(CTX), 1));
    }

}

void *FEIRToSKIR::visitFuncWork(FuncWork *work) {
    debug("in visitFuncWork");
    
    Constant *c = getOrInsertFunction("__streamit_" + currentStreamSpec->getName() + "_work", 
					   GetWorkFunctionType(CTX));
    lFunction *f = cast<lFunction>(c);
    syms.addEntry(syms.getScope() + "::work", work, f, true);
    currentBB = BasicBlock::Create(CTX,"entry", f);
    Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
    new BitCastInst(zero, zero->getType(), "alloca point", currentBB);

    // handle state
    {
	SKIRBuilder builder(currentBB);
	lFunction::arg_iterator args = f->arg_begin();
	Value *state = args;
	// point state at function argument
	currentStatePtr = builder.CreateBitCast(state, currentStatePtr->getType(), "state");
    }

    Statement *body = work->getBody();
    syms.pushScope("work");
    body->accept(this);
    syms.popScope();

    SKIRBuilder builder(currentBB);
    builder.CreateRet(ConstantInt::get(lType::getInt32Ty(CTX), 0));

    currentBB = NULL;
    return f;
}

// a program contains lists of streams, structs, and helper functions
void *FEIRToSKIR::visitProgram(Program *prog) {
    debug("in visitProgram");
    
    topLevelStream = 0;

    // build a symbol table for this program
    SymbolTableVisitor symsVisitor(&syms);
    symsVisitor.setVerbose(verbose);
    symsVisitor.visitProgram(prog);

    StreamSpecList *ssl = prog->getStreams();
    StreamSpecList::iterator ssiter;
    for (ssiter = ssl->begin(); ssiter != ssl->end(); ssiter++) {
	StreamSpec *ss = *ssiter;
	if (ss->getType() ==  StreamSpec::STREAM_GLOBAL) {
	    ss->accept(this);
	    continue;
	}
	StreamType *st = ss->getStreamType();
	sType *typeIn = st->getIn();
	sType *typeOut = st->getOut();
	if (typeIn->isVoid() && typeOut->isVoid()) {
	    topLevelStream = ss;
	    ss->accept(this);
	    break;
	}
    }
	
    FEPrintVisitor::visitProgram(prog);

    if (topLevelStream) {
	Constant* c = mod->getOrInsertFunction("main", lType::getVoidTy(CTX), NULL);
	lFunction *main = cast<lFunction>(c);
	BasicBlock *bb = BasicBlock::Create(CTX,"entry", main);
	Value *zero = ConstantInt::get(lType::getInt32Ty(CTX), 0);
	new BitCastInst(zero, zero->getType(), "alloca point", bb);

	SKIRBuilder builder(bb);

	Value *global_init = mod->getFunction("__streamit_TheGlobal_init");
	if (global_init) {
	    Value *nul = ConstantPointerNull::get(cast<PointerType>(GetVoidPtrType(CTX)));
	    builder.CreateCall(global_init, nul);
	}

	string top = topLevelStream->getName();
	Value *init = syms.getValue("streamit::"+top+"::init", true);
	Value *work = syms.getValue("streamit::"+top+"::work", true);

	Value *k = builder.CreateKernel(mod, init, work, NULL);
	builder.CreateKernelCall(mod, k, NULL, NULL);
	builder.CreateWait(mod, k);
	builder.CreateRetVoid();
    }

    if (verbose)
	mod->dump();
    return NULL;
}

//void *FEIRToSKIR::visitSCAnon(SCAnon *creator) { return NULL; }
//void *FEIRToSKIR::visitSCSimple(SCSimple *creator) { return NULL; }

void *FEIRToSKIR::visitSJDuplicate(SJDuplicate *sj) {
    debug("in visitSJDuplicate");

    StreamCreator *sc;
    ExpressionList *args = new ExpressionList;
    // number of children "n"
    args->push_back( new ExprVar(sj->getContext(), string("__num_children_ptr")) );
    sc = new SCSimple(sj->getContext(), "_splitdupN", new TypeList, args, NULL);
    return sc;
}

void *FEIRToSKIR::visitSJRoundRobin(SJRoundRobin *sj)
{
    debug("in visitSJRoundRobin");
    
    StreamCreator *sc;
    ExpressionList *args = new ExpressionList;
    // number of children "n"
    args->push_back( new ExprVar(sj->getContext(), string("__num_children_ptr")) );
    args->push_back( sj->getWeight() );
    string name;

    if (splitjoin == split) {
	IsZeroVisitor izv;
	if (sj->getWeight()->accept(&izv) != NULL) {
	    name = "_splitrr0";
	}
	else {
	    name = "_splitrrN";
	}
    } else {
	name = "_joinrrN";
    }
    sc = new SCSimple(sj->getContext(), name, new TypeList, args, NULL);
    return sc;
}

void *FEIRToSKIR::visitSJWeightedRR(SJWeightedRR *sj)
{
    StreamCreator *sc;

    ExpressionList *weights = sj->getWeights();
    ExpressionList *args = new ExpressionList;
    string name;
    if (weights->size() > 1) {
	//error("unsuported weighted rr");

	args->push_back( new ExprConstInt(weights->size()) );
	ExpressionList::iterator I = weights->begin(), E = weights->end();
	for (; I!=E; ++I) {
	    args->push_back( *I );
	} 
	if (weights->size() == 2) 
	    args->push_back( new ExprConstInt(0) );
	
	if (splitjoin == split) {
	    name = "_splitrrW";
	} else {
	    name = "_joinrrW";
	}
    }
    else {
	// number of children "n"
	args->push_back( new ExprVar(sj->getContext(), string("__num_children_ptr")) );
	args->push_back( weights->front() );

	if (splitjoin == split) {
	    name = "_splitrrN";
	} else {
	    name = "_joinrrN";
	}
    }

    sc = new SCSimple(sj->getContext(), name, new TypeList, args, NULL);
    return sc;
}

void *FEIRToSKIR::visitStmtAdd(StmtAdd *stmt)
{
    debug("in visitStmtAdd");

    SKIRBuilder builder(currentBB);
    StreamCreator *sc = stmt->getCreator();

    if (sc->isAnon()) {
	SCAnon *sca = (SCAnon*)sc;
	
	// first save the current state of things
	Value *savedStatePtr = currentStatePtr;
	BasicBlock *savedBB = currentBB;
	StreamSpec *savedStreamSpec = currentStreamSpec;
	string savedScope = syms.getScope();

	syms.setScope("streamit");
	
	StreamSpec *ss = sca->getSpec();
	ExpressionList *args = new ExpressionList;
	
	{
	    // copy parent parameters to anonymous child
	    // (i.e. extend part of the parent's scope into child)
	    // also pass parent fielddecls to child as read-only in this way
	    ParameterList *pl = new ParameterList;
	    pl->insert(pl->end(),
		       currentStreamSpec->getParams()->begin(),
		       currentStreamSpec->getParams()->end());
	   
	    FieldDeclList *fdl = currentStreamSpec->getFields();
	    FieldDeclList::iterator diter;
	    for (diter = fdl->begin(); diter != fdl->end(); diter++) {
		FieldDecl *fd = *diter;
		for (int i=0; i<fd->getNumFields(); i++) {
		    pl->push_back( new Parameter(fd->getType(i), fd->getName(i)) );
		}
	    }

	    ss = new StreamSpec(ss->getContext(), ss->getType(), ss->getStreamType(),
				ss->getName(), pl, ss->getFields(), ss->getFuncs());

	    ParameterList::iterator iter;
	    for (iter=pl->begin(); iter!=pl->end(); iter++) {
		Parameter *p = *iter;
		ExprVar *ev = new ExprVar(sca->getContext(), p->getName());
		args->push_back( ev );
		syms.addEntry(ss->getName()+"::"+p->getName(), ev, NULL);
	    }

	}
	sc = new SCSimple(sca->getContext(), ss->getName(), NULL, args, NULL);
	syms.addEntry(ss->getName(), (FENode*)ss, NULL);

	// jump to the needed streamspec
	ss->setParent(currentStreamSpec);
	ss->accept(this);
	
	// restore state
	currentStatePtr = savedStatePtr;
	currentBB = savedBB;
	currentStreamSpec = savedStreamSpec;
	syms.setScope(savedScope);
    }

    SCSimple *scs = (SCSimple*)sc;
    StreamSpec *child = (StreamSpec*)syms.getNode("streamit::"+scs->getName(), true);
    if (!child) error("undefined child stream: "+scs->getName());
    child->setParent(currentStreamSpec);
    
    // if these don't exist, the streamspec hasn't been visited yet
    Value *child_work = syms.getValue("streamit::"+scs->getName()+"::work", true);
    Value *child_init = syms.getValue("streamit::"+scs->getName()+"::init", true);
    if (!child_work && !child_init) {
	
	// first save the current state of things
	Value *savedStatePtr = currentStatePtr;
	BasicBlock *savedBB = currentBB;
	StreamSpec *savedStreamSpec = currentStreamSpec;
	string savedScope = syms.getScope();
	
	// jump to the needed streamspec
	syms.setScope("streamit");
	//syms.popScope(); // init
	//syms.popScope(); // stream name
	child->accept(this);
	
	// restore state
	currentStatePtr = savedStatePtr;
	currentBB = savedBB;
	currentStreamSpec = savedStreamSpec;
	syms.setScope(savedScope);		
	
	child_work = syms.getValue("streamit::"+scs->getName()+"::work", true);
	child_init = syms.getValue("streamit::"+scs->getName()+"::init", true);
    }
    assert(child_work && child_init && "undeclared child filter!");
    
    // create the child's args
    ParameterList *child_pl = child->getParams();
    ExpressionList *child_al = scs->getParams();
    size_t n = child_pl->size();
    if (n != child_al->size())
	error("number of params does not match number of args for "+scs->getName());
    
    // XXX specialize work function
    
    // get the types
    vector<const lType *> childParamTypes;
    ParameterList::iterator child_piter;
    for (child_piter = child_pl->begin(); child_piter != child_pl->end(); child_piter++) {
	Parameter *p = *child_piter;
	childParamTypes.push_back(tConvert.convert(p->getType()));
    }
    
    Value *kernel;
    if (n) {
	// allocate the param structure
	const lType *childParamStructType = StructType::get(CTX,childParamTypes);
	//Value *childParamsStruct = builder.CreateAlloca(childParamStructType);
	Value *childParamsStruct = new AllocaInst(childParamStructType, 
						  "", getAllocaPoint(currentBB->getParent()));

	
	// set the param struct values
	for (size_t i=0; i<n; i++) {
	    Value *ptr = builder.CreateStructGEP(childParamsStruct, i);
	    Value *v = (Value*)child_al->at(i)->accept(this);
	    v = matchTypes(v,cast<PointerType>(ptr->getType())->getElementType());
	    builder.CreateStore(v,ptr);
	}
    
	// create the child
	childParamsStruct = builder.CreateBitCast(childParamsStruct, GetVoidPtrType(CTX));
	kernel = builder.CreateKernel(mod,
				      child_init,
				      child_work,
				      childParamsStruct);
    } else {
	Value *nullPointer = ConstantPointerNull::get(cast<PointerType>(GetVoidPtrType(CTX)));
	kernel = builder.CreateKernel(mod,
				      child_init,
				      child_work,
				      nullPointer);
    }	
	
    
    // store the result of @llvm.skir.kernel into state->children[state->num_children]

    // load current value of num_children
    Value *idx[3] = { ConstantInt::get(lType::getInt32Ty(CTX), 0), NULL, NULL };
    idx[1] = syms.getValue("__num_children");
    Value *num_children_ptr = builder.CreateGEP(currentStatePtr, idx, idx+2);
    Value *num_children = builder.CreateLoad(num_children_ptr);

    // store kernel into children[num_children]
    idx[1] = syms.getValue("__children");
    idx[2] = num_children;
    Value *children_ptr = builder.CreateGEP(currentStatePtr, idx, idx+3);
    builder.CreateStore(kernel, children_ptr);

    // increment num_children
    num_children = builder.CreateAdd(num_children, ConstantInt::get(lType::getInt32Ty(CTX), 1));
    builder.CreateStore(num_children, num_children_ptr);

    return NULL;
}

void *FEIRToSKIR::visitStmtAssign(StmtAssign *stmt)
{
    debug("in visitStmtAssign");

    if (stmt->getOp()) {
	FEContext *c = stmt->getContext();
	int op = stmt->getOp();
	Expression *lhs = stmt->getLHS();
	Expression *rhs = stmt->getRHS();

	stmt = new StmtAssign(c, lhs, new ExprBinary(c, op, lhs, rhs));
	return stmt->accept(this);

    } else {
	inLHS = true;
	Value *lhs = (Value*)(stmt->getLHS()->accept(this));
	inLHS = false;
	
	Value *rhs = (Value*)(stmt->getRHS()->accept(this));
	
	SKIRBuilder builder(currentBB);
	rhs = matchTypes(rhs, cast<PointerType>(lhs->getType())->getElementType());
	builder.CreateStore(rhs, lhs);
	
	return (void *)rhs;
    }
}

// XXX block => new scope ??
// void *FEIRToSKIR::visitStmtBlock(StmtBlock *s) {
//     debug("in visitStmtBlock");
//     StatementList *sl = s->getStmts();
//     StatementList::iterator iter;
//     for (iter = sl->begin(); iter != sl->end(); iter++) {
// 	Statement *s = *iter;
// 	s->accept(this);
//     }
//     return NULL;
//}

//void *FEIRToSKIR::visitStmtBody(StmtBody *stmt) { return NULL; }

void *FEIRToSKIR::visitStmtBreak(StmtBreak *stmt) {
    SKIRBuilder builder(currentBB);
    builder.CreateBr(breakBB);
    lFunction *currentFunction = builder.GetInsertBlock()->getParent();
    currentBB = BasicBlock::Create(CTX,"dead", currentFunction);
    return NULL;
}

void *FEIRToSKIR::visitStmtContinue(StmtContinue *stmt) {
    SKIRBuilder builder(currentBB);
    builder.CreateBr(continueBB);
    lFunction *currentFunction = builder.GetInsertBlock()->getParent();
    currentBB = BasicBlock::Create(CTX,"dead", currentFunction);
    return NULL;
}

void *FEIRToSKIR::visitStmtDoWhile(StmtDoWhile *stmt) {
    SKIRBuilder builder(currentBB);

    lFunction *currentFunction = builder.GetInsertBlock()->getParent();
    BasicBlock *bodyBB = BasicBlock::Create(CTX,"body", currentFunction);
    BasicBlock *condBB = BasicBlock::Create(CTX,"cond");
    BasicBlock *doneBB = BasicBlock::Create(CTX,"done");

    builder.CreateBr(bodyBB);

    // body block
    breakBB = doneBB;
    continueBB = condBB;
    currentBB = bodyBB;
    if (stmt->getBody())
	stmt->getBody()->accept(this);
    bodyBB = currentBB;
    builder.SetInsertPoint(bodyBB);
    builder.CreateBr(condBB);
    breakBB = NULL;
    currentBB = NULL;

    // evaluate cond to true/false
    currentBB = condBB;
    currentFunction->getBasicBlockList().push_back(condBB);
    Value *cond = (Value *)stmt->getCond()->accept(this);
    builder.SetInsertPoint(condBB);
    const lType *t = cond->getType();
    if (t != lType::getInt1Ty(CTX)) {
	if (t->isIntegerTy())
	    cond = builder.CreateICmpNE(cond, ConstantInt::get(t, 0));
	else if (t->isFloatingPointTy())
	    cond = builder.CreateFCmpONE(cond, ConstantFP::get(t, 0));
    }
    builder.CreateCondBr(cond, bodyBB, doneBB);

    // exit block
    currentFunction->getBasicBlockList().push_back(doneBB);
    currentBB = doneBB;

    return NULL;
}

//void *FEIRToSKIR::visitStmtEmpty(StmtEmpty *stmt) { return NULL; }
//void *FEIRToSKIR::visitStmtEnqueue(StmtEnqueue *stmt) { return NULL; }
//void *FEIRToSKIR::visitStmtExpr(StmtExpr *stmt) { return NULL; }

void *FEIRToSKIR::visitStmtFor(StmtFor *stmt)
{
    SKIRBuilder builder(currentBB);
    if (stmt->getInit())
	stmt->getInit()->accept(this);

    lFunction *currentFunction = builder.GetInsertBlock()->getParent();
    BasicBlock *condBB = BasicBlock::Create(CTX,"cond", currentFunction);
    BasicBlock *bodyBB = BasicBlock::Create(CTX,"body");
    BasicBlock *incrBB = BasicBlock::Create(CTX,"incr");
    BasicBlock *doneBB = BasicBlock::Create(CTX,"done");

    builder.CreateBr(condBB);

    // evaluate cond to true/false
    currentBB = condBB;
    Value *cond = (Value *)stmt->getCond()->accept(this);
    builder.SetInsertPoint(condBB);
    const lType *t = cond->getType();
    if (t != lType::getInt1Ty(CTX)) {
	if (t->isIntegerTy())
	    cond = builder.CreateICmpNE(cond, ConstantInt::get(t, 0));
	else if (t->isFloatingPointTy())
	    cond = builder.CreateFCmpONE(cond, ConstantFP::get(t, 0));
    }
    builder.CreateCondBr(cond, bodyBB, doneBB);

    // body block
    breakBB = doneBB;
    continueBB = incrBB;
    currentFunction->getBasicBlockList().push_back(bodyBB);
    currentBB = bodyBB;
    if (stmt->getBody())
	stmt->getBody()->accept(this);
    bodyBB = currentBB;
    builder.SetInsertPoint(bodyBB);
    builder.CreateBr(incrBB);
    breakBB = NULL;
    currentBB = NULL;

    // increment block
    currentFunction->getBasicBlockList().push_back(incrBB);
    currentBB = incrBB;
    if (stmt->getIncr())
	stmt->getIncr()->accept(this);
    incrBB = currentBB;
    builder.SetInsertPoint(incrBB);
    builder.CreateBr(condBB);
    
    // exit block
    currentFunction->getBasicBlockList().push_back(doneBB);
    currentBB = doneBB;

    return NULL;
}

void *FEIRToSKIR::visitStmtIfThen(StmtIfThen *stmt)
{
    SKIRBuilder builder(currentBB);

    // evaluate cond to true/false
    Value *cond = (Value *)stmt->getCond()->accept(this);
    // handle cases like if (i)
    const lType *t = cond->getType();
    if (t != lType::getInt1Ty(CTX)) {
	if (t->isIntegerTy())
	    cond = builder.CreateICmpNE(cond, ConstantInt::get(t, 0));
	else if (t->isFloatingPointTy())
	    cond = builder.CreateFCmpONE(cond, ConstantFP::get(t, 0));
    }

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    lFunction *currentFunction = builder.GetInsertBlock()->getParent();
    BasicBlock *thenBB = BasicBlock::Create(CTX,"then", currentFunction);
    BasicBlock *elseBB = BasicBlock::Create(CTX,"else");
    BasicBlock *mergeBB = BasicBlock::Create(CTX,"ifcont");
    
    Statement *cons = stmt->getCons();
    Statement *alt = stmt->getAlt();

    builder.CreateCondBr(cond, cons ? thenBB : mergeBB, alt ? elseBB : mergeBB);

    // codegen then block
    if (cons) {
	currentBB = thenBB;
	cons->accept(this);
	thenBB = currentBB;
	builder.SetInsertPoint(thenBB);
	builder.CreateBr(mergeBB);
    }

    // codegen else block
    if (alt) {
	currentFunction->getBasicBlockList().push_back(elseBB);
	currentBB = elseBB;
	alt->accept(this);
	elseBB = currentBB;
	builder.SetInsertPoint(elseBB);
	builder.CreateBr(mergeBB);
    }

    // attach merge block
    currentBB = mergeBB;
    currentFunction->getBasicBlockList().push_back(mergeBB);

    return NULL;
}

void *FEIRToSKIR::visitStmtJoin(StmtJoin *stmt)
{
    splitjoin = join;
    StreamCreator *sc = (StreamCreator *)stmt->getJoiner()->accept(this);
    StmtAdd *add = new StmtAdd(stmt->getContext(), sc);
    visitStmtAdd(add);
    delete add;
    delete sc;
    return NULL;
}

//void *FEIRToSKIR::visitStmtLoop(StmtLoop *stmt) { return NULL; }

void *FEIRToSKIR::visitStmtPush(StmtPush *stmt) {
    debug("in visitStmtPush");
	
    //sType *t = currentStreamSpec->getStreamType()->getOut();
    Expression *e = stmt->getValue();
    Value *v = (Value *)e->accept(this);
    SKIRBuilder builder(currentBB);
    //Value *ptr = builder.CreateAlloca(v->getType());
    Value *ptr = new AllocaInst(v->getType(), "", getAllocaPoint(currentBB->getParent()));
    builder.CreateStore(v,ptr);
    Value *stream = ConstantInt::get(lType::getInt32Ty(CTX),0);
    return builder.CreatePush(mod, stream, ptr);
}

void *FEIRToSKIR::visitStmtReturn(StmtReturn *stmt) {
    error("unhandled return statement!",stmt);
    return NULL;
}

//void *FEIRToSKIR::visitStmtSendMessage(StmtSendMessage *stmt) { return NULL; }
//void *FEIRToSKIR::visitStmtHelperCall(StmtHelperCall *stmt) { return NULL; }

void *FEIRToSKIR::visitStmtSplit(StmtSplit *stmt)
{
    debug("in visitStmtSplit");

    splitjoin = split;
    StreamCreator *sc = (StreamCreator *)stmt->getSplitter()->accept(this);
    if (sc) {
	StmtAdd *add = new StmtAdd(stmt->getContext(), sc);
	visitStmtAdd(add);
	delete add;
	delete sc;
    }
    return NULL;
}
	

void *FEIRToSKIR::visitStmtVarDecl(StmtVarDecl *stmt)
{
    debug("in visitStmtVarDecl");

    int n = stmt->getNumVars();
    SKIRBuilder builder(currentBB);
    for (int i=0; i<n; i++) {

	sType *stype = stmt->getType(i);
	const lType *ltype = tConvert.convert(stype);
	string name = stmt->getName(i);
	Expression *init = stmt->getInit(i);

	// for struct streams
	currentStreamSpec->getFields()->push_back(new FieldDecl(stmt->getContext(),
								stype, name, init));

	Value *ptr = new AllocaInst(ltype, name.c_str(), getAllocaPoint(currentBB->getParent()));

	if (!init) {
	    //CreateZeroVisitor zv(stmt->getContext());
	    //init = zv.getZeroExprOfType(stype);
	    builder.CreateStore(Constant::getNullValue(ltype), ptr);
	}

	if (init) {
	    if (init->isArrayInit()) {
		doArrayInit(ptr, (ExprArrayInit*)init);
	    } else {
		Value *val = (Value*)init->accept(this);
		val = matchTypes(val, cast<PointerType>(ptr->getType())->getElementType());
		builder.CreateStore(val, ptr);
	    }
	}

	// save the result (Value*) of any init expr in the symbol table
	syms.addEntry(name, syms.getNode(name), ptr);
    }
    return NULL;
} 

void *FEIRToSKIR::visitStmtWhile(StmtWhile *stmt) {
    SKIRBuilder builder(currentBB);

    lFunction *currentFunction = builder.GetInsertBlock()->getParent();
    BasicBlock *condBB = BasicBlock::Create(CTX,"cond", currentFunction);
    BasicBlock *bodyBB = BasicBlock::Create(CTX,"body");
    BasicBlock *doneBB = BasicBlock::Create(CTX,"done");

    builder.CreateBr(condBB);

    // evaluate cond to true/false
    currentBB = condBB;
    Value *cond = (Value *)stmt->getCond()->accept(this);
    builder.SetInsertPoint(condBB);
    const lType *t = cond->getType();
    if (t != lType::getInt1Ty(CTX)) {
	if (t->isIntegerTy())
	    cond = builder.CreateICmpNE(cond, ConstantInt::get(t, 0));
	else if (t->isFloatingPointTy())
	    cond = builder.CreateFCmpONE(cond, ConstantFP::get(t, 0));
    }
    builder.CreateCondBr(cond, bodyBB, doneBB);

    // body block
    breakBB = doneBB;
    continueBB = condBB;
    currentFunction->getBasicBlockList().push_back(bodyBB);
    currentBB = bodyBB;
    if (stmt->getBody())
	stmt->getBody()->accept(this);
    bodyBB = currentBB;
    builder.SetInsertPoint(bodyBB);
    builder.CreateBr(condBB);
    breakBB = NULL;
    currentBB = NULL;

    // exit block
    currentFunction->getBasicBlockList().push_back(doneBB);
    currentBB = doneBB;

    return NULL;
}

void *FEIRToSKIR::visitStreamSpec(StreamSpec *spec) {
    debug("in visitStreamSpec");

    // if already visited
    if (syms.getValue("streamit::"+spec->getName()+"::init", true) != NULL)
	return NULL;

    currentStatePtr = NULL;
    currentBB = NULL;
    currentStreamSpec = spec;
    syms.pushScope(spec->getName());

    Function *init = spec->getInitFunc();
    switch (spec->getType()) {
    case StreamSpec::STREAM_PIPELINE:
	createPipeline(init, "_pipeline");
	break;
    case StreamSpec::STREAM_SPLITJOIN:
	createPipeline(init, "_splitjoin");
	break;
    default:
	visitFuncInit(init);
	break;
    }

    FunctionList *funcs = spec->getFuncs();
    FunctionList::iterator iter;
    for (iter = funcs->begin(); iter != funcs->end(); iter++) {
	sFunction *f = *iter;
	if (f != init) f->accept(this);
    }

    syms.popScope();
    currentStatePtr = NULL;
    currentStreamSpec = NULL;
    currentBB = NULL;

    return NULL;
}

//void *FEIRToSKIR::visitStreamType(StreamType *type) { return NULL; }
//void *FEIRToSKIR::visitOther(FENode *node) { return NULL; }

Value *
FEIRToSKIR::matchTypes(Value *v, const lType *ty) 
{
    const lType *vty = v->getType();
    if (vty == ty)
	return v;
    
    SKIRBuilder builder(currentBB);
    if (vty->isIntegerTy() && ty->isFloatingPointTy())
	return builder.CreateSIToFP(v,ty);

    if (vty->isFloatingPointTy() && ty->isIntegerTy())
	return builder.CreateFPToSI(v,ty);

    warn("warning: FEIRToSKIR::matchTypes: unhandled type");
    return v;    
}

Constant *
FEIRToSKIR::getOrInsertFunction(StringRef name, const FunctionType *type) 
{
#if 0
    lFunction *f = mod->getFunction(name);
    if (!f) {
	f =  lFunction::Create(type, GlobalVariable::InternalLinkage, name);
	mod->getFunctionList().push_back(f);
    }
#endif
    return mod->getOrInsertFunction(name, type);
}

void FEIRToSKIR::addBuiltins(void)
{
    // println
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "i") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "__streamit_println",
				     new TypePrimitive(TypePrimitive::TYPE_VOID),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::println", (FENode*)f, NULL, true);
    }    
    // print
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_STRING), "s") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "__streamit_print",
				     new TypePrimitive(TypePrimitive::TYPE_VOID),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::print", (FENode*)f, NULL, true);
    }    
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "__streamit_println_f",
				     new TypePrimitive(TypePrimitive::TYPE_VOID),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::println_f", (FENode*)f, NULL, true);
    }

    // sin
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "sinf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::sin", (FENode*)f, NULL, true);
    }

    // cos
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "cosf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::cos", (FENode*)f, NULL, true);
    }

    // atan
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "atanf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::atan", (FENode*)f, NULL, true);
    }

    // abs
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "fabs",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::abs", (FENode*)f, NULL, true);
    }

    // round
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "roundf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::round", (FENode*)f, NULL, true);
    }

    // sqrt
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "sqrtf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::sqrt", (FENode*)f, NULL, true);
    }

    // exp
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "expf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::exp", (FENode*)f, NULL, true);
    }

    // log
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "logf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::log", (FENode*)f, NULL, true);
    }

    // floor
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "f") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "floorf",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::floor", (FENode*)f, NULL, true);
    }

    // atan2
    {
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "x") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_FLOAT), "y") );
	sFunction *f = new sFunction(NULL,
				     Function::FUNC_NATIVE,
				     "atan2f",
				     new TypePrimitive(TypePrimitive::TYPE_FLOAT),
				     pl,
				     NULL, NULL, NULL, NULL);
	
	syms.addEntry("streamit::atan2", (FENode*)f, NULL, true);
    }

    // pipeline
    {
	Constant *c = getOrInsertFunction("__streamit__pipeline_init", 
					       GetInitFunctionType(CTX));
	syms.addEntry("streamit::_pipeline::init", NULL, c, true);
    }
    {
	Constant *c = getOrInsertFunction("__streamit__pipeline_work", 
					       GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_pipeline::work", NULL, c, true);
    }
    // splitjoin
    {
	Constant *c = getOrInsertFunction("__streamit__splitjoin_init", 
					       GetInitFunctionType(CTX));
	syms.addEntry("streamit::_splitjoin::init", NULL, c, true);
    }
    {
	Constant *c = getOrInsertFunction("__streamit__splitjoin_work", 
					       GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_splitjoin::work", NULL, c, true);
    }
    
    // split duplicate
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__splitdup_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_splitdup::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__splitdup_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_splitdup::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INTPTR), "noutput*") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_splitdup", pl, NULL, NULL);
	syms.addEntry("streamit::_splitdup", ss, NULL, true);
    }
    
    // split duplicate
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__splitdupN_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_splitdupN::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__splitdupN_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_splitdupN::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INTPTR), "noutput*") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_splitdupN", pl, NULL, NULL);
	syms.addEntry("streamit::_splitdupN", ss, NULL, true);
    }

    // weighted rr split (1 parameter)
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__splitrr1_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_splitrr1::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__splitrr1_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_splitrr1::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INTPTR), "noutput*") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "weight") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_splitrr1", pl, NULL, NULL);
	syms.addEntry("streamit::_splitrr1", ss, NULL, true);
    }
    // weighted rr split (1 parameter)
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__splitrrN_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_splitrrN::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__splitrrN_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_splitrrN::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INTPTR), "noutput*") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "weight") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_splitrrN", pl, NULL, NULL);
	syms.addEntry("streamit::_splitrrN", ss, NULL, true);
    }
    // weighted rr split (2-3 parameter)
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__splitrrW_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_splitrrW::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__splitrrW_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_splitrrW::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "num") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "arg0") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "arg1") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "arg2") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_splitrrN", pl, NULL, NULL);
	syms.addEntry("streamit::_splitrrW", ss, NULL, true);
    }

    // weighted rr split (1 parameter)
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__splitrr0_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_splitrr0::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__splitrr0_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_splitrr0::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INTPTR), "noutput*") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "weight") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_splitrr0", pl, NULL, NULL);
	syms.addEntry("streamit::_splitrr0", ss, NULL, true);
    }

    // weighted rr join (1 parameter)
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__joinrr1_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_joinrr1::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__joinrr1_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_joinrr1::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INTPTR), "ninput*") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "weight") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_joinrr1", pl, NULL, NULL);
	syms.addEntry("streamit::_joinrr1", ss, NULL, true);
    }
    // weighted rr join (1 parameter)
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__joinrrN_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_joinrrN::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__joinrrN_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_joinrrN::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INTPTR), "ninput*") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "weight") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_joinrrN", pl, NULL, NULL);
	syms.addEntry("streamit::_joinrrN", ss, NULL, true);
    }
    // weighted rr join (2-3 parameter)
    {
	Constant *c;
	c = getOrInsertFunction("__streamit__joinrrW_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::_joinrrW::init", NULL, c, true);
	c = getOrInsertFunction("__streamit__joinrrW_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::_joinrrW::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "num") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "arg0") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "arg1") );
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_INT), "arg2") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "_joinrrW", pl, NULL, NULL);
	syms.addEntry("streamit::_joinrrW", ss, NULL, true);
    }

    // Identity
    {
	Constant *c = getOrInsertFunction("__streamit_Identity_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::Identity::init", NULL, c, true);
	c = getOrInsertFunction("__streamit_Identity_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::Identity::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "Identity", pl, NULL, NULL);
	syms.addEntry("streamit::Identity", ss, NULL, true);
    }

    // filereaderInt32
    {
	Constant *c;
	c = getOrInsertFunction("__streamit_FileReaderInt32_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::FileReaderInt32::init", NULL, c, true);
	c = getOrInsertFunction("__streamit_FileReaderInt32_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::FileReaderInt32::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_STRING), "filename") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "FileReaderInt32", pl, NULL, NULL);
	syms.addEntry("streamit::FileReaderInt32", ss, NULL, true);
    }

    // filereaderInt32
    {
	Constant *c;
	c = getOrInsertFunction("__streamit_FileReaderFloat_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::FileReaderFloat::init", NULL, c, true);
	c = getOrInsertFunction("__streamit_FileReaderFloat_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::FileReaderFloat::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_STRING), "filename") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "FileReaderFloat", pl, NULL, NULL);
	syms.addEntry("streamit::FileReaderFloat", ss, NULL, true);
    }

    // filewriterInt32
    {
	Constant *c;
	c = getOrInsertFunction("__streamit_FileWriterInt32_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::FileWriterInt32::init", NULL, c, true);
	c = getOrInsertFunction("__streamit_FileWriterInt32_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::FileWriterInt32::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_STRING), "filename") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "FileWriterInt32", pl, NULL, NULL);
	syms.addEntry("streamit::FileWriterInt32", ss, NULL, true);
    }

    // filewriterFloat32
    {
	Constant *c;
	c = getOrInsertFunction("__streamit_FileWriterFloat_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::FileWriterFloat::init", NULL, c, true);
	c = getOrInsertFunction("__streamit_FileWriterFloat_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::FileWriterFloat::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	pl->push_back( new Parameter(new TypePrimitive(TypePrimitive::TYPE_STRING), "filename") );
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "FileWriterFloat", pl, NULL, NULL);
	syms.addEntry("streamit::FileWriterFloat", ss, NULL, true);
    }

    // DummySinkInt
    {
	Constant *c;
	c = getOrInsertFunction("__streamit_DummySinkInt_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::DummySinkInt::init", NULL, c, true);
	c = getOrInsertFunction("__streamit_DummySinkInt_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::DummySinkInt::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "DummySinkInt", pl, NULL, NULL);
	syms.addEntry("streamit::DummySinkInt", ss, NULL, true);
    }

    // DummySinkFloat
    {
	Constant *c;
	c = getOrInsertFunction("__streamit_DummySinkFloat_init", GetInitFunctionType(CTX));
	syms.addEntry("streamit::DummySinkFloat::init", NULL, c, true);
	c = getOrInsertFunction("__streamit_DummySinkFloat_work", GetWorkFunctionType(CTX));
	syms.addEntry("streamit::DummySinkFloat::work", NULL, c, true);
	ParameterList *pl = new ParameterList;
	StreamSpec *ss = new StreamSpec(NULL, StreamSpec::STREAM_FILTER,
					NULL, "DummySinkFloat", pl, NULL, NULL);
	syms.addEntry("streamit::DummySinkFloat", ss, NULL, true);
    }
}
