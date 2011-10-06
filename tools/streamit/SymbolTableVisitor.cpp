
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
