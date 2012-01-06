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

#include <llvm/Pass.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/CallSite.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Module.h>

#include <map>

using namespace llvm;

namespace {

template <class ArgIt> static CallInst 
*ReplaceCallWith(Function *f, CallInst *CI,
		 ArgIt ArgBegin, ArgIt ArgEnd) {
    
    SmallVector<Value *, 8> Args(ArgBegin, ArgEnd);
    CallInst *NewCI = CallInst::Create(f, Args.begin(), Args.end(), CI->getName(), CI);

    if (!CI->use_empty())
	CI->replaceAllUsesWith(NewCI);
    CI->eraseFromParent();

    return NewCI;
}

struct SKIRExtractStaticPass : public ModulePass {

    static char ID;
    SKIRExtractStaticPass() : ModulePass((intptr_t)&ID) {}

    typedef std::map< SKIRStreamInst *, std::pair<Value*, Value*>* > graph_map;
    graph_map g;

    bool runOnModule(Module &M)
    {
        for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
	    inst_iterator I = inst_begin(MI);
	    inst_iterator E = inst_end(MI);
	    while (I != E) {
		Instruction &inst = *I; ++I;
		if (SKIRCallInst *CI = dyn_cast<SKIRCallInst>(&inst))
		    visitSKIRCallInst(*CI);
	    }
	}

        outs() << "digraph {\n";
        for (graph_map::iterator GI = g.begin(), GE = g.end(); GI!=GE; ++GI) {
            std::pair<Value*, Value*> *p = GI->second;
            outs() << p->first->getName() << " -> " << p->second->getName() << "\n";
        }
        outs() << "}\n";
        return false;
    }

    void visitGEP(Value *work, GEPOperator *GEP, bool src)
    {
        if ( (GEP->getNumIndices() == 2) && (GEP->hasAllConstantIndices()) ) {
            ConstantInt *idx0 = cast<ConstantInt>(GEP->getOperand(1));
            //ConstantInt *idx1 = cast<ConstantInt>(GEP->getOperand(2));
            if (idx0->isZero()) {
                //uint64_t stream_index = idx1->getZExtValue();
                for (Value::use_iterator i = GEP->use_begin(), e = GEP->use_end(); i != e; ++i) {
                    if (StoreInst *SI = dyn_cast<StoreInst>(i)) {
                        Value *v = SI->getOperand(0)->stripPointerCasts();
                        if (SKIRStreamInst *strm = dyn_cast<SKIRStreamInst>(v)) {
                            std::pair<Value*, Value*> *p;
                            if (!g.count(strm)) {
                                p = new std::pair<Value*, Value*>;
                                p->first = p->second = 0;
                                g[strm] = p;
                            }
                            p = g.find(strm)->second;
                            if (src) p->first = work;
                            else     p->second = work;
                        }
                    }
                }
            }
            
        }
    }
    
    void visitSKIRCallInst(SKIRCallInst &CI)
    {
        Value *kernel = CI.getOperand(1)->stripPointerCasts();
        Value *ins = CI.getOperand(2)->stripPointerCasts();
        Value *outs = CI.getOperand(3)->stripPointerCasts();

        SKIRKernelInst *KI = dyn_cast<SKIRKernelInst>(kernel);
        if (KI) {
            Value *work = KI->getOperand(1)->stripPointerCasts();

            for (Value::use_iterator UI = outs->use_begin(), UE = outs->use_end(); UI != UE; ++UI)
                if (GEPOperator *GEP = dyn_cast<GEPOperator>(UI))
                    visitGEP(work, GEP, true);
            
            for (Value::use_iterator UI = ins->use_begin(), UE = ins->use_end(); UI != UE; ++UI)
                if (GEPOperator *GEP = dyn_cast<GEPOperator>(UI))
                    visitGEP(work, GEP, false);
        }
    }
};

char SKIRExtractStaticPass::ID = 0;
RegisterPass<SKIRExtractStaticPass> X("skir-extract-graph",
                                      "Extract static stream graphs from a SKIR program", false, false);

}

namespace llvm {

ModulePass *createSKIRExtractStaticPass()
{
    return new SKIRExtractStaticPass;
}

}
