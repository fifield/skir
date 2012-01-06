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
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Constants.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>

#include <SKIR/SKIRRuntime.h>
#include "SKIRRuntimeStream.h"
#include "SKIRRuntimeKernel.h"
#include "SKIRUtil.h"

#include <vector>
#include <string>

using namespace llvm;

namespace {

struct SKIRStreamOptsPass : public FunctionPass {

 private:
    SKIRRuntimeKernel *kernel;

    Module *mod;
    Module *inline_module;

 public:    
    static char ID;
    SKIRStreamOptsPass() : FunctionPass((intptr_t)&ID) { 
	inline_module = 0;
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
     }
    
    void setKernel(SKIRRuntimeKernel *k) {
	kernel = k;
    }

    bool specializeOperSize(Function &F)
    {
	if (!kernel) return false;
	//if (!kernel->is_const_idx) return false;

	Function *work = &F;
	//LLVMContext &CTX = F.getContext();
	bool r = false;

	inst_iterator prev, next_prev=inst_begin(work);
	for (inst_iterator I = inst_begin(work), E = inst_end(work); I != E;) {
	    Instruction *inst = &*I; prev = next_prev; next_prev = I; ++I;

	    if (CallInst *CI = dyn_cast<CallInst>(inst)) {
		if (!CI->getCalledFunction())
		    continue;
		std::string name(CI->getCalledFunction()->getName());
		std::string prefix("__SKIRRT_inline");
		std::string tmplt("_E_B_ILi0ELi0");

		if (name.find(prefix) == std::string::npos) continue;
		if (name.find(tmplt) == std::string::npos) continue;

		// stream index
		if (!dyn_cast<ConstantInt>(CI->getOperand(2))) continue;

		ConstantInt *idx_oper = cast<ConstantInt>(CI->getOperand(2));
		unsigned idx = idx_oper->getZExtValue();

		size_t elem_size;
		if (name.find("push") != std::string::npos)
		    elem_size = kernel->rt_outs[idx]->elem_size;
		else
		    elem_size = kernel->rt_ins[idx]->elem_size;

		std::stringstream ss;
		ss << "_E_B_ILi" << elem_size << "ELi" << STREAM_BUFFER_SIZE;
		name.replace( name.find(tmplt), tmplt.length(), ss.str() );

		if (!work->getParent()->getFunction(name)) {
		    assert(0);
		}
			
		Constant *F = getInlineCode(work->getParent(), name.c_str());
		User::op_iterator B = CI->op_begin(); ++B;
		CI = ReplaceCallWith(CI, F, B, CI->op_end() );
		
		r = true;

	    } // if callinst
	} //for

	return r;
    }

    virtual bool runOnFunction(Function &F) {
	//Value *vins;
	//Value *vouts;
	//std::vector<CallInst*> inline_sites;
	//LLVMContext &CTX = F.getContext();
	//Function *work = &F;

	mod = F.getParent();
	assert(mod && "F is required to have a parent module");
	
	// specialize for stream operation operand size
	bool ret = specializeOperSize(F);
	
	return ret;
    }
};
 
 char SKIRStreamOptsPass::ID = 0;
 RegisterPass<SKIRStreamOptsPass> X("skir-stream-opts-pass",
				    "SKIR", false, false);

}

namespace llvm {

FunctionPass *createSKIRStreamOptsPass()
{
    return new SKIRStreamOptsPass;
}

}

void
SKIRRuntime::addSKIRStreamOptsPass(SKIRRuntimeKernel *k)
{
    // add the pass 
    SKIRStreamOptsPass *pass = new SKIRStreamOptsPass();
    pass->setKernel(k);

    k->fpm->add(pass);
    k->fpm->add(createVerifierPass());
}

void
SKIRRuntime::runSKIRStreamOptsPass(SKIRRuntimeKernel *k)
{
    // run the pass 
    Function *F = k->work;
    FunctionPassManager PM(F->getParent());

    PM.add(new TargetData(F->getParent()));
	
    SKIRStreamOptsPass *pass = new SKIRStreamOptsPass();
    pass->setKernel(k);

    PM.add(pass);
    PM.add(createVerifierPass());

    {
	MutexGuard locked(k->cg->lock);
	PM.run(*F);
    }
}
