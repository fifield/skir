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

#include <iostream>
#include <fstream>

#include "StreamItLex.hpp"
#include "StreamItParserFE.hpp"

#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>

#include "FEIRToSKIR.hpp"

#include <SKIR/SKIRBuilder.h>

using namespace llvm;
using namespace streamit;
using namespace std;

#define lFunction llvm::Function
#define lType llvm::Type

#define out std::cout

extern "C" {

}

Program *
parseProgram(istream &in)
{
    ANTLR_USING_NAMESPACE(std);
    ANTLR_USING_NAMESPACE(antlr);
    
    StreamItLex lexer(in);
    StreamItParserFE parser(lexer);
    
    return parser.program();
}

int
main(int argc, char *argv[])
{
    string ofilename("-"), ifilename("-");
    bool verbose = false;

    for (int i=1; i<argc; i++) {
	if (!strncmp("-i", argv[i], 2))
	    ifilename = argv[++i];
	if (!strncmp("-o", argv[i], 2))
	    ofilename = argv[++i];
	if (!strncmp("-v", argv[i], 2))
	    verbose = true;
    }
    
    LLVMContext &CTX = getGlobalContext();
    Module *mod = new Module("streamit",CTX);
    mod->setDataLayout("e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128");
    mod->setTargetTriple("x86_64-unknown-linux-gnu");

    // open library module
    string skir_root(getenv("SKIR_OBJ_ROOT"));
    string libfile(skir_root+"/lib/streamit.bc");
    string errormsg;
    Module *lib;
    if (MemoryBuffer *buffer = MemoryBuffer::getFileOrSTDIN(libfile, &errormsg)) {
	lib = ParseBitcodeFile(buffer, CTX, &errormsg);
	delete buffer;
    } else {
	errs() << argv[0] << ": error opening library file: " << libfile << "\n";
	return 1;
    }
    if (lib == 0) {
	std::cerr << argv[0] << ": error opening library file:";
	if (errormsg.size())
	    std::cerr << errormsg << "\n";
	else
	    std::cerr << "bitcode didn't read correctly.\n";
	return 1;
    }
    // add library functions to output module
    Module::FunctionListType &funcs = lib->getFunctionList();
    Module::iterator I  = funcs.begin();
    Module::iterator IE = funcs.end();
    while(I != IE) {
	lFunction &func = *I;
	lFunction *F = mod->getFunction(func.getName());
	if (F) {
	    F->replaceAllUsesWith(&func);
	    F->eraseFromParent();
	}
	func.removeFromParent();
	mod->getFunctionList().push_back(&func);
	I = funcs.begin();
	IE = funcs.end();
    }
    
    mod->getGlobalList().splice( mod->getGlobalList().end(),
				 lib->getGlobalList() );

    FEIRToSKIR v(mod);
    v.setVerbose(verbose);

    Program *p;
    if (ifilename == "-") {
	p = parseProgram(std::cin);
    } else {
	ifstream ifile;
	ifile.open(ifilename.c_str());
	p = parseProgram(ifile);
    }

    p->accept(&v);
    verifyModule(*mod, PrintMessageAction);

    PassManager pm;
    pm.add(createPromoteMemoryToRegisterPass());
    pm.add(createDeadCodeEliminationPass());

    if (verbose)
	mod->dump();
#if 0
    if (ofilename != "-") {
	ofstream ofile;
	ofile.open(ofilename.c_str());
	ofile << mod;
    } else {
#endif
	outs() << *mod;
	//    }

    return 0;
}
