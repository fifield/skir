#ifndef _SYMBOLTABLE_HPP_
#define _SYMBOLTABLE_HPP_

#include <string>
#include <map>
#include <utility>
#include <llvm/Type.h>
#include <llvm/Value.h>
#include "nodes/FENode.hpp"

/*
  this class serves two purposes:
    1) map strings to <streamit::FENode*, llvm::Value*> pairs
    2) help keep track of current scope
*/
namespace streamit {

typedef std::pair<FENode*, llvm::Value*> symtbl_ent;
typedef std::string symtbl_key;
typedef std::map<symtbl_key, symtbl_ent> symtbl;

class SymbolTable
{
private:
    symtbl syms;
    string scope;

    symtbl_ent getEnt(std::string name, bool scoped);

public:
    SymbolTable(string scope) : scope(scope) { verbose = false; }

    // if scoped==true then name already contains scope information, i.e. 'filter::func::var'
    // if scoped==false then name contains no scope information, i.e. 'var'
    // if scoped==false, scope information is added based on current scope
    bool getExists(std::string name, bool scoped=false);
    FENode *getNode(std::string name, bool scoped=false);
    llvm::Value *getValue(std::string name, bool scoped=false);

    void addEntry(std::string name, FENode *node, llvm::Value *val, bool scoped=false);

    // enter scope s
    void pushScope(string s);
    // exit scope
    void popScope();
    // get/set current scope
    string getScope() { return scope; }
    void setScope(std::string s);

    bool verbose;
};

}

#endif
