/*
 * Copyright 2003 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
#ifndef _FEVISITOR_HPP_
#define _FEVISITOR_HPP_

// get NULL
#include <cstdlib>

namespace streamit {

class ExprArray;
class ExprArrayInit;
class ExprBinary;
class ExprComplex;
class ExprComposite;
class ExprConstBoolean;
class ExprConstChar;
class ExprConstFloat;
class ExprConstInt;
class ExprConstStr;
class ExprDynamicToken;
class ExprField;
class ExprFunCall;
class ExprHelperCall;
class ExprPeek;
class ExprPop;
class ExprRange;
class ExprTernary;
class ExprTypeCast;
class ExprUnary;
class ExprVar;
class FieldDecl;
class Function;
class FuncWork;
class Program;
class SCAnon;
class SCSimple;
class SJDuplicate;
class SJRoundRobin;
class SJWeightedRR;
class StmtAdd;
class StmtAssign;
class StmtBlock;
class StmtBody;
class StmtBreak;
class StmtContinue;
class StmtDoWhile;
class StmtEnqueue;
class StmtEmpty;
class StmtExpr;
class StmtFor;
class StmtIfThen;
class StmtJoin;
class StmtLoop;
class StmtPush;
class StmtReturn;
class StmtSendMessage;
class StmtHelperCall;
class StmtSplit;
class StmtVarDecl;
class StmtWhile;
class StreamSpec;
class StreamType;
class FENode;

/**
 * Visitor interface for StreamIt front-end nodes.  This class
 * implements part of the "visitor" design pattern for StreamIt
 * front-end nodes.  The pattern basically exchanges type structures
 * for function calls, so a different function in the visitor is
 * called depending on the run-time type of the object being visited.
 * Calling visitor methods returns some value, the type of which
 * depends on the semantics of the visitor in question.  In general,
 * you will create a visitor object, and then pass it to the
 * <code>FENode.accept()</code> method of the object in question.
 *
 * @author  David Maze &lt;dmaze@cag.lcs.mit.edu&gt;
 * @version $Id: FEVisitor.java,v 1.19 2006/08/23 23:01:08 thies Exp $
 */
class FEVisitor
{
public:
    virtual void *visitExprArray(ExprArray *exp) { return NULL; }
    virtual void *visitExprArrayInit(ExprArrayInit *exp) { return NULL; }
    virtual void *visitExprBinary(ExprBinary *exp) { return NULL; }
    virtual void *visitExprComplex(ExprComplex *exp) { return NULL; }
    virtual void *visitExprComposite(ExprComposite *exp) { return NULL; }
    virtual void *visitExprConstBoolean(ExprConstBoolean *exp) { return NULL; }
    virtual void *visitExprConstChar(ExprConstChar *exp) { return NULL; }
    virtual void *visitExprConstFloat(ExprConstFloat *exp) { return NULL; }
    virtual void *visitExprConstInt(ExprConstInt *exp) { return NULL; }
    virtual void *visitExprConstStr(ExprConstStr *exp) { return NULL; }
    virtual void *visitExprDynamicToken(ExprDynamicToken *exp) { return NULL; }
    virtual void *visitExprField(ExprField *exp) { return NULL; }
    virtual void *visitExprFunCall(ExprFunCall *exp) { return NULL; }
    virtual void *visitExprHelperCall(ExprHelperCall *exp) { return NULL; }
    virtual void *visitExprPeek(ExprPeek *exp) { return NULL; }
    virtual void *visitExprPop(ExprPop *exp) { return NULL; }
    virtual void *visitExprRange(ExprRange *exp) { return NULL; }
    virtual void *visitExprTernary(ExprTernary *exp) { return NULL; }
    virtual void *visitExprTypeCast(ExprTypeCast *exp) { return NULL; }
    virtual void *visitExprUnary(ExprUnary *exp) { return NULL; }
    virtual void *visitExprVar(ExprVar *exp) { return NULL; }
    virtual void *visitFieldDecl(FieldDecl *field) { return NULL; }
    virtual void *visitFunction(Function *func) { return NULL; }
    virtual void *visitFuncWork(FuncWork *func) { return NULL; }
    virtual void *visitProgram(Program *prog) {	return NULL; }
    virtual void *visitSCAnon(SCAnon *creator) { return NULL; }
    virtual void *visitSCSimple(SCSimple *creator) { return NULL; }
    virtual void *visitSJDuplicate(SJDuplicate *sj) { return NULL; }
    virtual void *visitSJRoundRobin(SJRoundRobin *sj) { return NULL; }
    virtual void *visitSJWeightedRR(SJWeightedRR *sj) { return NULL; }
    virtual void *visitStmtAdd(StmtAdd *stmt) { return NULL; }
    virtual void *visitStmtAssign(StmtAssign *stmt) { return NULL; }
    virtual void *visitStmtBlock(StmtBlock *stmt) { return NULL; }
    virtual void *visitStmtBody(StmtBody *stmt) { return NULL; }
    virtual void *visitStmtBreak(StmtBreak *stmt) { return NULL; }
    virtual void *visitStmtContinue(StmtContinue *stmt) { return NULL; }
    virtual void *visitStmtDoWhile(StmtDoWhile *stmt) { return NULL; }
    virtual void *visitStmtEmpty(StmtEmpty *stmt) { return NULL; }
    virtual void *visitStmtEnqueue(StmtEnqueue *stmt) { return NULL; }
    virtual void *visitStmtExpr(StmtExpr *stmt) { return NULL; }
    virtual void *visitStmtFor(StmtFor *stmt) { return NULL; }
    virtual void *visitStmtIfThen(StmtIfThen *stmt) { return NULL; }
    virtual void *visitStmtJoin(StmtJoin *stmt) { return NULL; }
    virtual void *visitStmtLoop(StmtLoop *stmt) { return NULL; }
    virtual void *visitStmtPush(StmtPush *stmt) { return NULL; }
    virtual void *visitStmtReturn(StmtReturn *stmt) { return NULL; }
    virtual void *visitStmtSendMessage(StmtSendMessage *stmt) { return NULL; }
    virtual void *visitStmtHelperCall(StmtHelperCall *stmt) { return NULL; }
    virtual void *visitStmtSplit(StmtSplit *stmt) { return NULL; }
    virtual void *visitStmtVarDecl(StmtVarDecl *stmt) { return NULL; }
    virtual void *visitStmtWhile(StmtWhile *stmt) { return NULL; }
    virtual void *visitStreamSpec(StreamSpec *spec) { return NULL; }
    virtual void *visitStreamType(StreamType *type) { return NULL; }
    virtual void *visitOther(FENode *node) { return NULL; }
};

}

#endif
