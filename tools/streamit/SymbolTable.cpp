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

#include "SymbolTable.hpp"

using namespace std;
using namespace streamit;
using llvm::Value;

//#define sType streamit::Type
//#define lType llvm::Type
//#define sFunction streamit::Function
//#define lFunction llvm::Function

void
SymbolTable::pushScope(string name)
{
    scope = scope + "::" + name;
    if (verbose) cout << "push " << scope << endl;
}

void
SymbolTable::popScope()
{
    string::size_type n = scope.rfind("::");
    if (n != string::npos)
	scope.erase(n,scope.size());
    if (verbose) cout << "pop " << scope << endl;
}

void
SymbolTable::setScope(string s)
{ 
    if (verbose) cout << "set " << scope << " -> " << s << endl;
    scope = s;
}

symtbl_ent
SymbolTable::getEnt(string name, bool scoped)
{
    if (!scoped)
	name = getScope() + "::" + name;

    if (verbose) cout << "ST:  lookup " << name << endl;

    symtbl::iterator iter = syms.find(name);
    if (iter == syms.end()) {
	// N.B. this only works when there is always a 
	// named outer scope, i.e. 'streamit::...'
	string::size_type a = name.find("::");
	string::size_type b = name.rfind("::");
	string id = name.substr(b,name.size());
	if (a != b) {
	    // remove inner scope and retry
	    string old_scope = name.substr(0,b);
	    string new_scope = old_scope.substr(0,old_scope.rfind("::"));
	    return getEnt(new_scope + id, true);
	} else {
	    // try global scope last (exclude init and work fns)
	    if ( (name.find("::init") == string::npos) && (name.find("::work") == string::npos)) {
		name = "streamit::TheGlobal" + id;
		if (verbose) cout << "ST:  lookup " << name << endl;
		iter = syms.find(name);
	    }
	    if (iter == syms.end())
		return symtbl_ent(NULL,NULL);
	}
    }
    if (verbose) cout<< "ST:  found " << name << endl;
    symtbl_ent ent = (*iter).second;
    return ent;
}

Value *
SymbolTable::getValue(string name, bool scoped)
{
    symtbl_ent ent = getEnt(name, scoped);
    return ent.second;
}

FENode *
SymbolTable::getNode(string name, bool scoped)
{
    symtbl_ent ent = getEnt(name, scoped);
    return ent.first;
}

bool
SymbolTable::getExists(string name, bool scoped)
{
    symtbl_ent ent = getEnt(name, scoped);
    if ((ent.first == NULL) && (ent.second == NULL))
	return false;
    return true;
}

void
SymbolTable::addEntry(string name, FENode *node, Value *val, bool scoped)
{
    if (!scoped)
	name = getScope() + "::" + name;
    symtbl_ent ent(node, val);
    syms[name] = ent;
    if (verbose) cout << "add " << name << " (" << hex << node << "," << val << ")" << endl;
}

