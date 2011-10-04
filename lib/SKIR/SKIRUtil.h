#include <llvm/Module.h>
#include <llvm/Instructions.h>
#include <llvm/PassManager.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/InstIterator.h>
#include "llvm/TypeSymbolTable.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"


#include <vector>
#include <string.h>

#include "SKIRRuntimeKernel.h"

using namespace llvm;

namespace {


#if 0
#include <fstream>
    ofstream o;
    o.open((kernel->work->getName().str()+"_rt.ll").c_str());
    raw_os_ostream oo(o);
    kernel->work->print(oo, 0);
    o.close();
#endif


// BEGIN FROM llvm/lib/CodeGen/IntrinsicLowering.cpp
/// ReplaceCallWith - This function is used when we want to lower an intrinsic
/// call to a call of an external function.  This handles hard cases such as
/// when there was already a prototype for the external function, and if that
/// prototype doesn't match the arguments we expect to pass in.
template <class ArgIt> static CallInst *
ReplaceCallWith(const char *NewFn, CallInst *CI, 
		ArgIt ArgBegin, ArgIt ArgEnd, const Type *RetTy, Constant *&FCache) 
{
    if (!FCache) {
	// If we haven't already looked up this function, check to see if the
	// program already contains a function with this name.
	Module *M = CI->getParent()->getParent()->getParent();
	// Get or insert the definition now.
	std::vector<const Type *> ParamTys;
	for (ArgIt I = ArgBegin; I != ArgEnd; ++I)
	    ParamTys.push_back((*I)->getType());
	FCache = M->getOrInsertFunction(NewFn, FunctionType::get(RetTy, ParamTys, false));
    }
    
    SmallVector<Value *, 8> Args(ArgBegin, ArgEnd);
    CallInst *NewCI = CallInst::Create(FCache, Args.begin(), Args.end(), CI->getName(), CI);

    if (!CI->use_empty())
	CI->replaceAllUsesWith(NewCI);
    CI->eraseFromParent();

    return NewCI;
}

template <class ArgIt> static CallInst *
ReplaceCallWith(CallInst *CI, Constant *F, ArgIt ArgBegin, ArgIt ArgEnd)
{
    assert(F);

    SmallVector<Value *, 8> Args(ArgBegin, ArgEnd);
    CallInst *NewCI = CallInst::Create(F, Args.begin(), Args.end(), CI->getName(), CI);

    if (!CI->use_empty())
	CI->replaceAllUsesWith(NewCI);
    CI->eraseFromParent();

    return NewCI;
}

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

static Module *inline_module = 0;

Constant *
getInlineCode(Module *mod, const char *Fn)
{
    assert(mod);
     
     if (Constant *C = mod->getFunction(Fn))
	 return C;
     
     if (!inline_module) {
	 std::string skir_root(getenv("SKIR_ROOT"));
	 std::string filename(skir_root+"/skir/build/lib/inline_stream_ops.bc");
	 std::string errormsg;
	 if (MemoryBuffer *buffer = MemoryBuffer::getFileOrSTDIN(filename, &errormsg)) {
	     inline_module = ParseBitcodeFile(buffer, mod->getContext(), &errormsg);
	     delete buffer;
	 }
	 
	 if (inline_module == 0) {
	     if (errormsg.size())
		 errs() << errormsg << "\n";
	     else
		 errs() << "bitcode didn't read correctly.\n";
	     assert(0);
	     return 0;
	 }
	 
	 mod->getFunctionList().splice( mod->getFunctionList().end(),
					inline_module->getFunctionList() );
	 
	 mod->getGlobalList().splice( mod->getGlobalList().end(),
				      inline_module->getGlobalList() );
	 
     }
     
     if (Constant *C = mod->getFunction(Fn))
	 return C;
     
     /* handle the globals pointing to template/macro
	the generated code in inline_stream_ops.bc
     */
     if (GlobalVariable *GV = mod->getNamedGlobal(Fn)) {
	 if (Constant *C = GV->getInitializer())
	     return getInlineCode(mod, C->getName().str().c_str());
     }
     
     errs() << "Fn: " << Fn << " not found.\n";

     return 0;
}

Function *
makeWorkFromSkel(SKIRRuntimeKernel *k, const char *skel_name, Module *skel_mod=0)
{
    if (!skel_mod) skel_mod = k->work->getParent();

    Function *skel = dyn_cast<Function>(getInlineCode(skel_mod, skel_name));
    assert(skel && "skeleton function not found");
    
    std::string s(skel_name), new_name;
    size_t pos = s.rfind("_");
    if (pos != std::string::npos)
	new_name = std::string(k->work->getName()) + s.substr(pos);

    //if (Function *F = skel_mod->getFunction(new_name))
    //return F;

    skel = CloneFunction(skel);
    skel->setName(new_name);

    for (inst_iterator I = inst_begin(skel), E = inst_end(skel); I != E; ) {
	Instruction *inst = &*I; ++I;
	if (CallInst *CI = dyn_cast<CallInst>(inst)) {
	    // inline the original work function
	    if (CI->getCalledFunction()->getName() == "__SKIRRT_workfn_extern" ||
		CI->getCalledFunction()->getName() == "__SKIRRT_workfn_extern0" ||
		CI->getCalledFunction()->getName() == "__SKIRRT_workfn_extern1") {
		CI->setCalledFunction(k->work);
		assert( InlineFunction(CI) );
		return skel;
	    }
	}
    }
    
    return skel;
}

void
CloneModuleInto(const Module *srcM, Module *dstM)
{
    DenseMap<const Value*, Value*> ValueMap;

    // Copy all of the type symbol table entries over.
    const TypeSymbolTable &TST = srcM->getTypeSymbolTable();
    for (TypeSymbolTable::const_iterator TI = TST.begin(), TE = TST.end(); TI != TE; ++TI)
	dstM->addTypeName(TI->first, TI->second);

    // Copy all of the dependent libraries over.
    for (Module::lib_iterator I = srcM->lib_begin(), E = srcM->lib_end(); I != E; ++I)
	dstM->addLibrary(*I);

    // Loop over all of the global variables, making corresponding globals in the
    // new module.  Here we add them to the ValueMap and to the new Module.  We
    // don't worry about attributes or initializers, they will come later.
    //
    for (Module::const_global_iterator I = srcM->global_begin(), E = srcM->global_end(); 
	 I != E; ++I) {
	GlobalVariable *GV = new GlobalVariable(*dstM, 
						I->getType()->getElementType(),
						false,
						GlobalValue::ExternalLinkage, 0,
						I->getName());
	GV->setAlignment(I->getAlignment());
	ValueMap[I] = GV;
    }
    
    // Loop over the functions in the module, making external functions as before
    for (Module::const_iterator I = srcM->begin(), E = srcM->end(); I != E; ++I) {
	Function *NF = dstM->getFunction(I->getName());
	if (!NF) {
	    NF = Function::Create(cast<FunctionType>(I->getType()->getElementType()),
				  GlobalValue::ExternalLinkage, I->getName(), dstM);
	    NF->copyAttributesFrom(I);
	    ValueMap[I] = NF;
	}
	else {
		ValueMap[I] = NF;
	    if (NF->isIntrinsic()) {
		//ValueMap[I] = NF;
	    } else {
		;//errs() << "WARNING: function '" << I->getName() << "' already exists, skipping\n";
	    }
	}
    }
    
    // Loop over the aliases in the module
    for (Module::const_alias_iterator I = srcM->alias_begin(), E = srcM->alias_end();
	 I != E; ++I)
	ValueMap[I] = 
	    new GlobalAlias(I->getType(), GlobalAlias::ExternalLinkage,
			    I->getName(), NULL, dstM);
    
    
    // Now that all of the things that global variable initializer can refer to
    // have been created, loop through and copy the global variable referrers
    // over...  We also set the attributes on the global now.
    //
    for (Module::const_global_iterator I = srcM->global_begin(), E = srcM->global_end();
	 I != E; ++I) {
	GlobalVariable *GV = cast<GlobalVariable>(ValueMap[I]);
	if (I->hasInitializer())
	    GV->setInitializer(cast<Constant>(MapValue(I->getInitializer(),
						       ValueMap)));
	GV->setLinkage(I->getLinkage());
	GV->setThreadLocal(I->isThreadLocal());
	GV->setConstant(I->isConstant());
    }

    // Similarly, copy over function bodies now...
    //
    for (Module::const_iterator I = srcM->begin(), E = srcM->end(); I != E; ++I) {
	if (ValueMap[I] == 0) continue;
	Function *F = dyn_cast<Function>(ValueMap[I]);

	if (!I->isDeclaration()) {
	    Function::arg_iterator DestI = F->arg_begin();
	    for (Function::const_arg_iterator J = I->arg_begin(); J != I->arg_end();
		 ++J) {
		DestI->setName(J->getName());
		ValueMap[J] = DestI++;
	    }
	    
	    SmallVector<ReturnInst*, 8> Returns;  // Ignore returns cloned.
	    CloneFunctionInto(F, I, ValueMap, Returns);
	}
	
	F->setLinkage(I->getLinkage());
    }

    // And aliases
    for (Module::const_alias_iterator I = srcM->alias_begin(), E = srcM->alias_end();
	 I != E; ++I) {
	GlobalAlias *GA = cast<GlobalAlias>(ValueMap[I]);
	GA->setLinkage(I->getLinkage());
	if (const Constant* C = I->getAliasee())
	    GA->setAliasee(cast<Constant>(MapValue(C, ValueMap)));
    }
}

}    
