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

#ifndef _FEIRTOSKIR_HPP_
#define _FEIRTOSKIR_HPP_

#include "nodes/Nodes.hpp"
#include "nodes/FEPrintVisitor.hpp"
#include "SymbolTable.hpp"
#include "SymbolTableVisitor.hpp"
#include "TypeConvert.hpp"
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

namespace streamit {

class FEIRToSKIR : public FEPrintVisitor
{
public:
    FEIRToSKIR(llvm::Module *m) : syms("streamit"), CTX(m->getContext()), tConvert(m->getContext())
    { 
	mod = m;
	inLHS = false;
	currentBB = 0;
	currentStreamSpec = 0;
	currentStatePtr = 0;
	verbose = false;
	addBuiltins();
    }

    void *visitExprArray(ExprArray *exp);
    void *visitExprArrayInit(ExprArrayInit *exp);
    void *visitExprBinary(ExprBinary *exp);
//     void *visitExprComplex(ExprComplex *exp);
//     void *visitExprComposite(ExprComposite *exp);
    void *visitExprConstBoolean(ExprConstBoolean *exp);
    void *visitExprConstChar(ExprConstChar *exp);
    void *visitExprConstFloat(ExprConstFloat *exp);
    void *visitExprConstInt(ExprConstInt *exp);
    void *visitExprConstStr(ExprConstStr *exp);
//     void *visitExprDynamicToken(ExprDynamicToken *exp);
//     void *visitExprField(ExprField *exp);
    void *visitExprFunCall(ExprFunCall *exp);
//     void *visitExprHelperCall(ExprHelperCall *exp);
    void *visitExprPeek(ExprPeek *exp);
    void *visitExprPop(ExprPop *exp);
//     void *visitExprRange(ExprRange *exp);
    void *visitExprTernary(ExprTernary *exp);
    void *visitExprTypeCast(ExprTypeCast *exp);
    void *visitExprUnary(ExprUnary *exp);
    void *visitExprVar(ExprVar *exp);
    void *visitFieldDecl(FieldDecl *field);
    void *visitFunction(Function *func);
    void *visitFuncInit(Function *func);
    void *visitFuncWork(FuncWork *func);
    void *visitProgram(Program *prog);
//     void *visitSCAnon(SCAnon *creator);
//     void *visitSCSimple(SCSimple *creator);
    void *visitSJDuplicate(SJDuplicate *sj);
    void *visitSJRoundRobin(SJRoundRobin *sj);
    void *visitSJWeightedRR(SJWeightedRR *sj);
    void *visitStmtAdd(StmtAdd *stmt);
    void *visitStmtAssign(StmtAssign *stmt);
//     void *visitStmtBlock(StmtBlock *stmt);
//     void *visitStmtBody(StmtBody *stmt);
    void *visitStmtBreak(StmtBreak *stmt);
    void *visitStmtContinue(StmtContinue *stmt);
    void *visitStmtDoWhile(StmtDoWhile *stmt);
//     void *visitStmtEmpty(StmtEmpty *stmt);
//     void *visitStmtEnqueue(StmtEnqueue *stmt);
//     void *visitStmtExpr(StmtExpr *stmt);
    void *visitStmtFor(StmtFor *stmt);
    void *visitStmtIfThen(StmtIfThen *stmt);
    void *visitStmtJoin(StmtJoin *stmt);
//     void *visitStmtLoop(StmtLoop *stmt);
    void *visitStmtPush(StmtPush *stmt);
    void *visitStmtReturn(StmtReturn *stmt);
//     void *visitStmtSendMessage(StmtSendMessage *stmt);
//     void *visitStmtHelperCall(StmtHelperCall *stmt);
    void *visitStmtSplit(StmtSplit *stmt);
    void *visitStmtVarDecl(StmtVarDecl *stmt);
    void *visitStmtWhile(StmtWhile *stmt);
    void *visitStreamSpec(StreamSpec *spec);
//     void *visitStreamType(StreamType *type);
//     void *visitOther(FENode *node);

    SymbolTable syms;
    
    void setVerbose(bool b) { syms.verbose = verbose = b; }

private:

    void debug(std::string s) { 
	if (!verbose) return;
	std::cout << s << std::endl;
    }
    void print(std::string s) {
	if (!verbose) return;
	std::cout << "in "; debug(s+"*");
    }

    void error(std::string s, FENode *n=0) { 
	if (n) std::cout << n->getContext()->getLocation() << ": ";
	std::cout << s << std::endl;
	assert(0);
    }

    void warn(std::string s, FENode *n=0) { 
	if (n) std::cerr << n->getContext()->getLocation() << ": ";
	std::cerr << s << std::endl;
	assert(0);
    }

    llvm::Value *matchTypes(llvm::Value *v, const llvm::Type *ty);
    void doArrayInit(llvm::Value *v, ExprArrayInit *init);

    void createPipeline(Function *init, string worker);

     // add built-in functions to symbol table
    void addBuiltins(void);

    llvm::Constant *getOrInsertFunction(llvm::StringRef name, const llvm::FunctionType *ty);

    llvm::Module *mod;

    llvm::LLVMContext &CTX;

    // keep track of various things as we move down the AST
    // should be "callee-saved" when jumping between streamspecs
    StreamSpec *currentStreamSpec;
    // should be "callee-saved" when jumping between init/work functions
    llvm::Value *currentStatePtr;
    llvm::BasicBlock *currentBB;

    // targets for continue and break statements
    llvm::BasicBlock *continueBB;
    llvm::BasicBlock *breakBB;

    // are we in the left hand side of an assignment?
    bool inLHS;

    // streamit types -> llvm types
    TypeConvert tConvert;

    StreamSpec *topLevelStream;
    
    bool verbose;

    bool splitjoin;
};

}

#endif
