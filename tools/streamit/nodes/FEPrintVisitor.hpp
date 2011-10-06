// jf
#ifndef _FEPRINTVISITOR_HPP_
#define _FEPRINTVISITOR_HPP_

#include "FEVisitor.hpp"
#include "Nodes.hpp"

#include <iostream>

namespace streamit {

class FEPrintVisitor : public FEVisitor
{
private:

public:
    virtual void print(std::string s) { std::cout << s << std::endl; }

    virtual void *visitExprArray(ExprArray *exp) {
	print(__FUNCTION__);
	return NULL;
    }
    // XXX todo
    virtual void *visitExprArrayInit(ExprArrayInit *exp) {
	print(__FUNCTION__);
	return NULL;
    }
    virtual void *visitExprBinary(ExprBinary *exp) {
	print(__FUNCTION__);
	exp->getLeft()->accept(this);
	exp->getRight()->accept(this);
	return NULL;
    }
    virtual void *visitExprComplex(ExprComplex *exp) {
	print(__FUNCTION__);
	exp->getRealExpr()->accept(this);
	exp->getImagExpr()->accept(this);
	return NULL;
    }
    virtual void *visitExprComposite(ExprComposite *exp) {
	print(__FUNCTION__);
	exp->getX()->accept(this);
	exp->getY()->accept(this);
	Expression *e;
	e = exp->getZ();
	if (e) e->accept(this);
	e = exp->getW();
	if (e) e->accept(this);
	return NULL;
    }
    virtual void *visitExprConstBoolean(ExprConstBoolean *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitExprConstChar(ExprConstChar *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitExprConstFloat(ExprConstFloat *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitExprConstInt(ExprConstInt *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitExprConstStr(ExprConstStr *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitExprDynamicToken(ExprDynamicToken *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitExprField(ExprField *exp) {
	print(__FUNCTION__);
	exp->getLeft()->accept(this);
	return NULL;
    }
    virtual void *visitExprFunCall(ExprFunCall *exp) {
	print(__FUNCTION__);
	ExpressionList *el = exp->getParams();
	ExpressionList::iterator iter;
	for (iter = el->begin(); iter != el->end(); ++iter) {
	    Expression *e = *iter;
	    e->accept(this);
	}
	return NULL;
    }
    virtual void *visitExprHelperCall(ExprHelperCall *exp) {
	print(__FUNCTION__);
	ExpressionList *el = exp->getParams();
	ExpressionList::iterator iter;
	for (iter = el->begin(); iter != el->end(); ++iter) {
	    Expression *e = *iter;
	    e->accept(this);
	}
	return NULL;
    }
    virtual void *visitExprPeek(ExprPeek *exp) {
	print(__FUNCTION__);
	exp->getExpr()->accept(this);
	return NULL;
    }
    virtual void *visitExprPop(ExprPop *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitExprRange(ExprRange *exp) {
	print(__FUNCTION__);
	exp->getMin()->accept(this);
	exp->getAve()->accept(this);
	exp->getMax()->accept(this);
	return NULL;
    }
    virtual void *visitExprTernary(ExprTernary *exp) {
	print(__FUNCTION__);
	exp->getA()->accept(this);	exp->getB()->accept(this);
	exp->getC()->accept(this);
	return NULL;
    }
    virtual void *visitExprTypeCast(ExprTypeCast *exp) {
	print(__FUNCTION__);
	exp->getExpr()->accept(this);
	return NULL;
    }
    virtual void *visitExprUnary(ExprUnary *exp) {
	print(__FUNCTION__);
	exp->getExpr()->accept(this);
	return NULL;
    }
    virtual void *visitExprVar(ExprVar *exp) { print(__FUNCTION__); return NULL; }
    virtual void *visitFieldDecl(FieldDecl *field) {
	print(__FUNCTION__);
	ExpressionList *el = field->getInits();
	ExpressionList::iterator iter;
	for (iter = el->begin(); iter != el->end(); ++iter) {
	    Expression *e = *iter;
	    if (e) e->accept(this);
	}
	return NULL;
    }
    virtual void *visitFunction(Function *func) {
	print(__FUNCTION__);
	Expression *e;
	e = func->getPeekRate();
	if (e) e->accept(this);
	e = func->getPopRate();
	if (e) e->accept(this);
	e = func->getPushRate();
	if (e) e->accept(this);
	func->getBody()->accept(this);
	return NULL;
    }
    virtual void *visitFuncWork(FuncWork *func) {
	print(__FUNCTION__);
	Expression *e;
	e = func->getPeekRate();
	if (e) e->accept(this);
	e = func->getPopRate();
	if (e) e->accept(this);
	e = func->getPushRate();
	if (e) e->accept(this);
	func->getBody()->accept(this);
	return NULL;
    }
    virtual void *visitProgram(Program *prog) 
    {
	print(__FUNCTION__);
	
	StreamSpecList *ssl = prog->getStreams();
	StreamSpecList::iterator ssiter;
	for (ssiter = ssl->begin(); ssiter != ssl->end(); ++ssiter) {
	    StreamSpec *ss = *ssiter;
	    ss->accept(this);
	}
	
	TypeHelperList *thl = prog->getHelpers();
	TypeHelperList::iterator thiter;
	for (thiter = thl->begin(); thiter != thl->end(); ++thiter) {
	    TypeHelper *th = *thiter;
	    int n = th->getNumFuncs();
	    for (int i=0; i<n; i++) {
		Function *f = th->getFunction(i);
		f->accept(this);
	    }
	}
	return NULL;
    }
    // XXX portals for this and other SC
    virtual void *visitSCAnon(SCAnon *creator) {
	print(__FUNCTION__);
	creator->getSpec()->accept(this);
	return NULL;
    }
    virtual void *visitSCSimple(SCSimple *creator) {
	print(__FUNCTION__);
	ExpressionList *el = creator->getParams();
	ExpressionList::iterator iter;
	for (iter = el->begin(); iter != el->end(); ++iter) {
	    Expression *e = *iter;
	    e->accept(this);
	}	    
	return NULL;
    }
    virtual void *visitSJDuplicate(SJDuplicate *sj) { print(__FUNCTION__); return NULL; }
    virtual void *visitSJRoundRobin(SJRoundRobin *sj) {
	print(__FUNCTION__);
	sj->getWeight()->accept(this);
	return NULL;
    }
    virtual void *visitSJWeightedRR(SJWeightedRR *sj) {
	print(__FUNCTION__);
	ExpressionList *el = sj->getWeights();
	ExpressionList::iterator iter;
	for (iter = el->begin(); iter != el->end(); ++iter) {
	    Expression *e = *iter;
	    e->accept(this);
	}	    
	return NULL;
    }
    virtual void *visitStmtAdd(StmtAdd *stmt) {
	print(__FUNCTION__);
	stmt->getCreator()->accept(this);
	return NULL;
    }
    virtual void *visitStmtAssign(StmtAssign *stmt) {
	print(__FUNCTION__);
	stmt->getLHS()->accept(this);
	stmt->getRHS()->accept(this);
	return NULL;
    }
    virtual void *visitStmtBlock(StmtBlock *stmt) {
	print(__FUNCTION__);
	StatementList *sl = stmt->getStmts();
	StatementList::iterator iter;
	for (iter = sl->begin(); iter != sl->end(); ++iter) {
	    Statement *s = *iter;
	    s->accept(this);
	}
	return NULL;
    }
    virtual void *visitStmtBody(StmtBody *stmt) {
	print(__FUNCTION__);
	stmt->getCreator()->accept(this);
	return NULL;
    }
    virtual void *visitStmtBreak(StmtBreak *stmt) { print(__FUNCTION__); return NULL; }
    virtual void *visitStmtContinue(StmtContinue *stmt) { print(__FUNCTION__); return NULL; }
    virtual void *visitStmtDoWhile(StmtDoWhile *stmt) {
	print(__FUNCTION__);
	stmt->getCond()->accept(this);
	stmt->getBody()->accept(this);
	return NULL;
    }
    virtual void *visitStmtEmpty(StmtEmpty *stmt) { print(__FUNCTION__); return NULL; }
    virtual void *visitStmtEnqueue(StmtEnqueue *stmt) { print(__FUNCTION__); return NULL; }
    virtual void *visitStmtExpr(StmtExpr *stmt) {
	print(__FUNCTION__);
	stmt->getExpression()->accept(this);
	return NULL;
    }
    virtual void *visitStmtFor(StmtFor *stmt) {
	print(__FUNCTION__);
	Statement *s;
	Expression *e;
	s = stmt->getInit();
	if (s) s->accept(this);
	e = stmt->getCond();
	if (e) e->accept(this);
	s = stmt->getIncr();
	if (s) s->accept(this);
	s = stmt->getBody();
	if (s) s->accept(this);
	return NULL;
    }
    virtual void *visitStmtIfThen(StmtIfThen *stmt) {
	print(__FUNCTION__);
	stmt->getCond()->accept(this);
	Statement *s;
	s = stmt->getCons();
	if (s) s->accept(this);
	s = stmt->getAlt();
	if (s) s->accept(this);
	return NULL;
    }
    virtual void *visitStmtJoin(StmtJoin *stmt) {
	print(__FUNCTION__);
	stmt->getJoiner()->accept(this);
	return NULL;
    }
    virtual void *visitStmtLoop(StmtLoop *stmt) {
	print(__FUNCTION__);
	stmt->getCreator()->accept(this);
	return NULL;
    }
    virtual void *visitStmtPush(StmtPush *stmt) {
	print(__FUNCTION__);
	stmt->getValue()->accept(this);
	return NULL;
    }
    virtual void *visitStmtReturn(StmtReturn *stmt) {
	print(__FUNCTION__);
	stmt->getValue()->accept(this);
	return NULL;
    }
    virtual void *visitStmtSendMessage(StmtSendMessage *stmt) { print(__FUNCTION__); return NULL; }
    virtual void *visitStmtHelperCall(StmtHelperCall *stmt) {
	print(__FUNCTION__);
	ExpressionList *el = stmt->getParams();
	ExpressionList::iterator iter;
	for (iter = el->begin(); iter != el->end(); ++iter) {
	    Expression *e = *iter;
	    e->accept(this);
	}
	return NULL;
    }
    virtual void *visitStmtSplit(StmtSplit *stmt) {
	print(__FUNCTION__);
	stmt->getSplitter()->accept(this);
	return NULL;
    }
    virtual void *visitStmtVarDecl(StmtVarDecl *stmt) {
	print(__FUNCTION__);
	ExpressionList *el = stmt->getInits();
	ExpressionList::iterator iter;
	for (iter = el->begin(); iter != el->end(); ++iter) {
	    Expression *e = *iter;
	    if (e) e->accept(this);
	}
	return NULL;
    }
    virtual void *visitStmtWhile(StmtWhile *stmt) {
	print(__FUNCTION__);
	stmt->getCond()->accept(this);
	stmt->getBody()->accept(this);
	return NULL;
    }
    virtual void *visitStreamSpec(StreamSpec *spec) {
	print(__FUNCTION__);
	FieldDeclList *fdl = spec->getFields();
	FieldDeclList::iterator diter;
	for (diter = fdl->begin(); diter != fdl->end(); ++diter) {
	    FieldDecl *fd = *diter;
	    if (fd) fd->accept(this);
	}
	FunctionList *funcs = spec->getFuncs();
	FunctionList::iterator fiter;
	for (fiter = funcs->begin(); fiter != funcs->end(); ++fiter) {
	    Function *f = *fiter;
	    if (f) f->accept(this);
	}
	return NULL;
    }
    virtual void *visitStreamType(StreamType *type) { print(__FUNCTION__); return NULL; }
    virtual void *visitOther(FENode *node) { print(__FUNCTION__); return NULL; }
};

}
#endif
