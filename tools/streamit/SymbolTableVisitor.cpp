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

#include "SymbolTableVisitor.hpp"

using namespace streamit;
using namespace std;

void *
SymbolTableVisitor::visitProgram(Program *prog)
{
    // XXX todo: handle struct types
    FEPrintVisitor::visitProgram(prog);
    return NULL;
}

void *
SymbolTableVisitor::visitStreamSpec(StreamSpec *spec)
{
    string scope = syms->getScope();
    if (spec->getName() == "anon") {
	spec->setName( createAnonName() );
	syms->setScope("streamit");
    }
    string name = spec->getName();

    syms->addEntry(name, spec, NULL);
    syms->pushScope(name);

    ParameterList::iterator iter;
    ParameterList *pl = spec->getParams();
    for (iter = pl->begin(); iter != pl->end(); iter++) {
	Parameter *p = *iter;
	FEContext *c = spec->getContext();
	string n = p->getName();
	Type *t = p->getType();
	syms->addEntry(p->getName(), (FENode*)(new StmtVarDecl(c, t, n, 0)), 0);
    }

    FEPrintVisitor::visitStreamSpec(spec);

    syms->setScope(scope);
    return NULL;
}

void *
SymbolTableVisitor::visitFuncWork(FuncWork *func)
{
    syms->pushScope("work");
    FEPrintVisitor::visitFuncWork(func);
    syms->popScope();

    return NULL;
}

void *
SymbolTableVisitor::visitFunction(Function *func)
{
    if (func->getCls() == Function::FUNC_INIT)
	syms->pushScope("init");
    else
	syms->pushScope(func->getName());
    FEPrintVisitor::visitFunction(func);
    syms->popScope();
    return NULL;
}

void *
SymbolTableVisitor::visitStmtVarDecl(StmtVarDecl *stmt)
{
    for (int i=0; i<stmt->getNumVars(); i++)
	// XXX could create a new var for each
	syms->addEntry(stmt->getName(i), stmt, NULL);
    FEPrintVisitor::visitStmtVarDecl(stmt);
    return NULL;
}

void *
SymbolTableVisitor::visitFieldDecl(FieldDecl *field)
{
    for (int i=0; i<field->getNumFields(); i++)
	// XXX could create a new decl for each
	syms->addEntry(field->getName(i), field, NULL);
    FEPrintVisitor::visitFieldDecl(field);
    return NULL;
}

string
SymbolTableVisitor::createAnonName()
{
    static int n = 0;
    n++;
    stringstream ss;
    ss << "_anon" << n;
    return ss.str();
}
