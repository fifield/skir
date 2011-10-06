#ifndef _SYMBOLTABLEVISITOR_HPP_
#define _SYMBOLTABLEVISITOR_HPP_

#include "nodes/FEPrintVisitor.hpp"
#include "SymbolTable.hpp"

#include <iostream>
#include <string>

namespace streamit {

class SymbolTableVisitor : public FEPrintVisitor
{
private:
    SymbolTable *syms;
    bool verbose;

    // get a unique name
    string createAnonName();

public:
    void setVerbose(bool b) { verbose = b; }
    void print(std::string s) { if (verbose) {std::cout << "STV:  "; FEPrintVisitor::print(s);} }

    SymbolTableVisitor(SymbolTable *_syms) {
	syms = _syms;
	verbose = false;
    }

    //void *visitExprArray(ExprArray *exp);
    //void *visitExprArrayInit(ExprArrayInit *exp);
    //void *visitExprBinary(ExprBinary *exp);
    //void *visitExprComplex(ExprComplex *exp);
    //void *visitExprComposite(ExprComposite *exp);
    //void *visitExprConstBoolean(ExprConstBoolean *exp);
    //void *visitExprConstChar(ExprConstChar *exp);
    //void *visitExprConstFloat(ExprConstFloat *exp);
    //void *visitExprConstInt(ExprConstInt *exp);
    //void *visitExprConstLong(ExprConstLong *exp);
    //void *visitExprConstStr(ExprConstStr *exp);
    //void *visitExprDynamicToken(ExprDynamicToken *exp);
    //void *visitExprField(ExprField *exp);
    //void *visitExprFunCall(ExprFunCall *exp);
    //void *visitExprHelperCall(ExprHelperCall *exp);
    //void *visitExprPeek(ExprPeek *exp);
    //void *visitExprPop(ExprPop *exp);
    //void *visitExprRange(ExprRange *exp);
    //void *visitExprTernary(ExprTernary *exp);
    //void *visitExprTypeCast(ExprTypeCast *exp);
    //void *visitExprUnary(ExprUnary *exp);
    //void *visitExprVar(ExprVar *exp);
    void *visitFieldDecl(FieldDecl *field);
    void *visitFunction(Function *func);
    void *visitFuncWork(FuncWork *func);
    void *visitProgram(Program *prog);
    //void *visitSCAnon(SCAnon *creator);
    //void *visitSCSimple(SCSimple *creator);
    //void *visitSJDuplicate(SJDuplicate *sj);
    //void *visitSJRoundRobin(SJRoundRobin *sj);
    //void *visitSJWeightedRR(SJWeightedRR *sj);
    //void *visitStmtAdd(StmtAdd *stmt);
    //void *visitStmtAssign(StmtAssign *stmt);
    //void *visitStmtBlock(StmtBlock *stmt);
    //void *visitStmtBody(StmtBody *stmt);
    //void *visitStmtBreak(StmtBreak *stmt);
    //void *visitStmtContinue(StmtContinue *stmt);
    //void *visitStmtDoWhile(StmtDoWhile *stmt);
    //void *visitStmtEmpty(StmtEmpty *stmt);
    //void *visitStmtEnqueue(StmtEnqueue *stmt);
    //void *visitStmtExpr(StmtExpr *stmt);
    //void *visitStmtFor(StmtFor *stmt);
    //void *visitStmtIfThen(StmtIfThen *stmt);
    //void *visitStmtJoin(StmtJoin *stmt);
    //void *visitStmtLoop(StmtLoop *stmt);
    //void *visitStmtPush(StmtPush *stmt);
    //void *visitStmtReturn(StmtReturn *stmt);
    //void *visitStmtSendMessage(StmtSendMessage *stmt);
    //void *visitStmtHelperCall(StmtHelperCall *stmt);
    //void *visitStmtSplit(StmtSplit *stmt);
    void *visitStmtVarDecl(StmtVarDecl *stmt);
    //void *visitStmtWhile(StmtWhile *stmt);
    void *visitStreamSpec(StreamSpec *spec);
    //void *visitStreamType(StreamType *type);
    //void *visitOther(FENode *node);
};

}
#endif
