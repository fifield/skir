
#include <llvm/Pass.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include "llvm/Transforms/Scalar.h"
#include "llvm/Instruction.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include <SKIR/SKIRRuntime.h>
#include "SKIRRuntimeKernel.h"
#include "SKIRKernelInfoPass.h"

#include <string.h>
#include <string>
#include <set>

using namespace llvm;

char SKIRKernelInfo::ID = 0;
static RegisterPass<SKIRKernelInfo> X("skir-kernel-info",
				      "SKIR kernel info", false, false);

namespace llvm {
FunctionPass *createSKIRKernelInfo()
{
    return new SKIRKernelInfo;
}
}

void
SKIRKernelInfo::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<PostDominatorTree>();
    AU.addRequired<ScalarEvolution>();
    AU.addRequired<LoopInfo>();
    AU.setPreservesAll();
}

void
SKIRKernelInfo::init(SKIRRuntimeKernel *k)
{
    nins = -2;
    nouts = -2;
    memset(ins, 0, sizeof(Value *)*MAX_STREAMS);
    memset(outs, 0, sizeof(Value *)*MAX_STREAMS);
    memset(peeks, 0, sizeof(Value *)*MAX_STREAMS);
    memset(peeks_rate, 0, sizeof(Value *)*MAX_STREAMS);
    kernel = k;
    computed_rates = false;
}

void 
SKIRKernelInfo::print(raw_ostream &O, const Module*)
{
    int nins = getNumIns();
    int nouts = getNumOuts();
    
    //errs() << "getNumIns():  " << nins << "\n";
    //errs() << "getNumOuts():  " << nouts << "\n";

    for (int i=0; i<nins; i++) {
	Value *v = getPopRate(i);
	O << "in rate " << i << ": ";
	if (v) O << *v << "\n";
	else O << "unknown\n";
    }

    for (int i=0; i<nouts; i++) {
	Value *v = getPushRate(i);
	O << "out rate " << i << ": ";
	if (v) O << *v << "\n";
	else O << "unknown\n";
    }
}

#if 0
void
SKIRKernelInfo::findByIdx(unsigned idx, inst_list_t &input_list, inst_list_t &output_list)
{
    for(inst_list_t::iterator I = input_list.begin(), E = input_list.end(); I!=E; ++I) {
	Instruction *i = *I;
	Value *stream = i->getOperand(1);
	if (ConstantInt *stream_idx = dyn_cast<ConstantInt>(stream)) {
	    if (stream_idx->getZExtValue() == idx)
		output_list.push_back(i);
	}
    }
}
#endif

bool
SKIRKernelInfo::tryKnownRates(Function &work)
{
    size_t pos;

    if (!kernel)
	return false;

    const std::string name(kernel->base_work->getNameStr());

    pos = name.find("split_rr_work_N_A_ILi");
    if (pos != std::string::npos) {
	pos += strlen("split_rr_work_N_A_ILi");
	std::string nout_str(name.substr(pos, name.find("ELi", pos)-pos));
	int nout = atoi(nout_str.c_str());
	pos += nout_str.length()+3;
	std::string arg_str(name.substr(pos, name.find("E",pos)-pos));
	int arg = atoi(arg_str.c_str());
	
	for (int i=0; i<nout; i++)
	    kernel->rt_outs[i]->setPushRate(arg);
	kernel->rt_ins[0]->setPopRate(arg*nout);
	kernel->is_fixed_rate = true;
	kernel->has_push = kernel->has_pop = true;
	//errs() << name << " " << arg << " " << nout << "\n";
	return true;
    }

    pos = name.find("join_rr_work_N_A_ILi");
    if (pos != std::string::npos) {
	pos += strlen("join_rr_work_N_A_ILi");
	std::string nin_str(name.substr(pos, name.find("ELi", pos)-pos));
	int nin = atoi(nin_str.c_str());
	pos += nin_str.length()+3;
	std::string arg_str(name.substr(pos, name.find("E",pos)-pos));
	int arg = atoi(arg_str.c_str());
	
	for (int i=0; i<nin; i++)
	    kernel->rt_ins[i]->setPopRate(arg);
	kernel->rt_outs[0]->setPushRate(arg*nin);
	kernel->is_fixed_rate = true;
	kernel->has_push = kernel->has_pop = true;
	//errs() << name << " " << arg << " " << nin << "\n";
	return true;
    }

    pos = name.find("split_dup_work_N_ILi");
    if (pos != std::string::npos) {
	pos += strlen("split_dup_work_N_ILi");
	std::string nout_str(name.substr(pos, name.find("E", pos)-pos));
	int nout = atoi(nout_str.c_str());
	
	for (int i=0; i<nout; i++)
	    kernel->rt_outs[i]->setPushRate(1);
	kernel->rt_ins[0]->setPopRate(nout);
	kernel->is_fixed_rate = true;
	kernel->has_push = kernel->has_pop = true;
	//errs() << name << " " << nout << "\n";
	return true;
    }

    return false;
}

// return true if the store is side effect free.
// define the only side effect free store to be a store to the stack,
// assume that such an address is calculated using an alloca instruction
// and a combination of getelemptr and bitcastinst.  (this might be improved
// by using real alias analysis to see if an address can alias addresses other
// than the stack)
unsigned
checkStoreAddress(std::set<Value*> &visited, Value *v)
{    
    if (visited.count(v))
	return 0;
    visited.insert(v);

    // define the only side effect free store to be a store to the stack
    if (isa<AllocaInst>(v))
	return 0x10;

    // if we're storing through an argument (i.e. to kernel state)
    if (isa<Argument>(v))
	return 1;

    // if an instruction isn't an alloca instruction,
    // then recurse on the operands to find out what we have.
    if (Instruction *inst = dyn_cast<Instruction>(v)) {
	    
	if (isa<BitCastInst>(inst) || isa<IntToPtrInst>(inst) || 
	    isa<PtrToIntInst>(inst) || isa<GetElementPtrInst>(inst))  {
		return checkStoreAddress(visited, inst->getOperand(0));
	}
	
	if (isa<BinaryOperator>(inst)) {
	    return (checkStoreAddress(visited, inst->getOperand(0)) |
		    checkStoreAddress(visited, inst->getOperand(1)) );
	}
	
	if (PHINode *PHI = dyn_cast<PHINode>(inst)) {
	    int i = PHI->getNumIncomingValues();
	    unsigned ret = 0;
	    while(i--) ret |= checkStoreAddress(visited, PHI->getIncomingValue(i));
	    return ret;
	}

	return 1;
    }

    if (isa<Constant>(v))
	return 0;

    
    assert(0 && "unhandled llvm::Value type");
    return 1;
}

//
//
bool 
SKIRKernelInfo::runOnFunction(Function &work)
{
    if (!kernel) return false;
    if (kernel->opt_only) return false;

    bool is_stateful = false;

    // - build lists of all the pop, peek and push instructions
    // - a kernel is hierarchical if it contains call, kernel, or stream intrinsics
    for (inst_iterator I = inst_begin(work), E = inst_end(work); I != E; ++I) {
	Instruction *inst = &*I;
	if (isa<SKIRIntrinsic>(inst)) {
	    if (isa<SKIRPushInst>(inst)) push_list.push_back(inst);
	    else if (isa<SKIRPopInst>(inst)) pop_list.push_back(inst);
	    else if (isa<SKIRPeekInst>(inst)) peek_list.push_back(inst);
	    else if (isa<SKIRCallInst>(inst)) kernel->is_hier = true;
	    else if (isa<SKIRKernelInst>(inst)) kernel->is_hier = true;
	    else if (isa<SKIRStreamInst>(inst)) kernel->is_hier = true;
	}
	else if (CallInst *CI = dyn_cast<CallInst>(inst)) {
	    if (dyn_cast<Function>(CI->getCalledFunction())) {
		std::string name(CI->getCalledFunction()->getName());
		if (!name.find("__SKIRRT_call") ||
		    !name.find("__SKIRRT_kernel") ||
		    !name.find("__SKIRRT_stream") )
		    kernel->is_hier = true;
	    }
	}
	else if (isa<StoreInst>(inst)) {
	    std::set<Value*> visited;
	    unsigned result = checkStoreAddress(visited, inst->getOperand(1));
	    is_stateful = (result != 0x10);
	}
    }

    kernel->is_stateful = is_stateful;
    kernel->has_push = (push_list.size() > 0);
    kernel->has_pop = (pop_list.size() > 0);
    kernel->has_peek = (peek_list.size() > 0);
    kernel->is_hier = kernel->is_hier && !(kernel->has_push || kernel->has_pop || kernel->has_peek);

    //std::cout << kernel->work->getName().str() << " stateful: " << is_stateful << "\n";

    // find static input/output rates if possible,
    is_fixed_rate = true;

    // if the number of streams identified in the kernel 
    // don't match the number passed to the kernel at runtime,
    // then say it's not fixed rate
    int nin = std::min(kernel->nins, getNumIns());
    if (getNumIns() && nin != kernel->nins)
	is_fixed_rate = false;
    int nout = std::min(kernel->nouts, getNumOuts());
    if (getNumOuts() && nout != kernel->nouts)
	is_fixed_rate = false;

    // try to compute the pop rate for each input stream
    for (int i=0; i<nin; i++) {
	assert(kernel->rt_ins[i]);
	Value *v = getPopRate(i);
	if (!v) { is_fixed_rate = false; continue; }
	if (ConstantInt *ci = dyn_cast<ConstantInt>(v))
	    kernel->rt_ins[i]->setPopRate(ci->getZExtValue());
	else is_fixed_rate = false;
	// offset + 1
	if (peek_list.size()) {
	    Value *v = getPeekRate(i);
	    kernel->rt_ins[i]->setPeekRate(cast<ConstantInt>(v)->getZExtValue() + 1);
	} else 
	    kernel->rt_ins[i]->setPeekRate(0);
    }

    // try to compute the push rate for each input stream
    for (int i=0; i<nout; i++) {
	assert(kernel->rt_outs[i]);
	Value *v = getPushRate(i);
	if (!v) { is_fixed_rate = false; continue; }
	if (ConstantInt *ci = dyn_cast<ConstantInt>(v))
	    kernel->rt_outs[i]->setPushRate(ci->getZExtValue());
	else is_fixed_rate = false;
    }

    kernel->is_fixed_rate = is_fixed_rate;
    kernel->is_const_idx = is_fixed_rate;
    if (!is_fixed_rate) tryKnownRates(work);

    return false;
}

// return the number distinct input streams accessed in the work function
// if the number can't be determined statically, return -1
int
SKIRKernelInfo::getNumIns(void)
{
    if (nins != -2) return nins;

    nins = -1;

    std::set<unsigned> idx_set;

    for(inst_list_t::iterator I = pop_list.begin(), E = pop_list.end(); I!=E; ++I) {
	Instruction *pop = *I;
	Value *stream = pop->getOperand(1);
	if (!isa<ConstantInt>(stream))
	    return nins; // -1
	ConstantInt *stream_idx = cast<ConstantInt>(stream);
	idx_set.insert(stream_idx->getZExtValue());
    }

    nins = idx_set.size();

    return nins;
}

// return the number distinct output streams accessed in the work function
// if the number can't be determined statically, return -1
int
SKIRKernelInfo::getNumOuts(void)
{
    if (nouts != -2) return nouts;
    
    nouts = -1;

    std::set<unsigned> idx_set;

    for(inst_list_t::iterator I = push_list.begin(), E = push_list.end(); I!=E; ++I) {
	Instruction *push = *I;
	Value *stream = push->getOperand(1);
	if (!isa<ConstantInt>(stream)) 
	    return nouts;
	ConstantInt *stream_idx = cast<ConstantInt>(stream);
	idx_set.insert(stream_idx->getZExtValue());
    }

    nouts = idx_set.size();

    return nouts;
}

// return a value representing the rate for this instruction
Value *
SKIRKernelInfo::getRate(Instruction *inst, Value *streams[MAX_STREAMS])
{
    assert(isa<SKIRIntrinsic>(inst));

    BasicBlock *BB = inst->getParent();
    Function *F = BB->getParent();
    BasicBlock *entryBB = &F->getEntryBlock();

    PostDominatorTree *PDT = &getAnalysis<PostDominatorTree>();
    LoopInfo *LI = &getAnalysis<LoopInfo>();

    int s = 0;
    if (ConstantInt *idxCI = dyn_cast<ConstantInt>(inst->getOperand(1)))
	s = idxCI->getZExtValue();
    else
	return 0;

    Loop *L = LI->getLoopFor(BB);

    // the simplest case is an unconditional intrinsic,
    // with a constant index, that is not in a loop.
    if (!L) {
	if (PDT->dominates(BB, entryBB)) {
	    int rate = 0;
	    if (streams[s]) {
		ConstantInt *c = cast<ConstantInt>(streams[s]);
		rate = c->getZExtValue();
	    }
	    streams[s] = ConstantInt::get(Type::getInt32Ty(BB->getContext()),rate+1);
	}
	return streams[s];
    }

    while (L) {
	// require contant trip count
	unsigned const_trip_cnt = L->getSmallConstantTripCount();
	if (!const_trip_cnt) {
	    streams[s] = 0;
	    return 0;
	}
	
	uint64_t rate = 0;
	if (streams[s])
	    rate = cast<ConstantInt>(streams[s])->getZExtValue();
	streams[s] = ConstantInt::get(Type::getInt32Ty(BB->getContext()),rate + const_trip_cnt);

	L = L->getParentLoop();
    }
    return streams[s];
}
bool override;
// find the max offset for the given peek instruction
Value *
SKIRKernelInfo::getRateOffset(Instruction *inst, Value *streams[MAX_STREAMS])
{
    assert(isa<SKIRPeekInst>(inst));

    LLVMContext &CTX = inst->getContext();

    int s = 0;
    ConstantInt *idxCI = dyn_cast<ConstantInt>(inst->getOperand(1));
    if (idxCI) s = idxCI->getZExtValue();

#define FAIL(s) { streams[s] = 0; is_fixed_rate = false; return 0; }

    override = false;
    // test to meet the same requirements as push & pop
    if (!override && !getRate(inst, peeks_rate)) {
	if (idxCI) FAIL(s);
	return 0;
    }

    LoopInfo *LI = &getAnalysis<LoopInfo>();

    Value *offset = inst->getOperand(3);
    unsigned depth = LI->getLoopDepth(inst->getParent());
    if (depth==0) {
	if (ConstantInt *c = dyn_cast<ConstantInt>(offset)) {
	    uint64_t rate = 0;
	    if (streams[s])
		rate = cast<ConstantInt>(streams[s])->getZExtValue();
	    rate = std::max(c->getZExtValue(), rate);
	    streams[s] = ConstantInt::get(Type::getInt32Ty(CTX),rate);
	} 
	else FAIL(s);

	return streams[s];
    }

    ScalarEvolution *SE = &getAnalysis<ScalarEvolution>();

    if (!SE->isSCEVable(offset->getType()))
	FAIL(s);

    if (const SCEV *expr = SE->getSCEV(offset)) {
	const SCEVAddRecExpr *addrec = dyn_cast<SCEVAddRecExpr>(expr);
	if (!addrec)
	    FAIL(s);

	// brute force - find the max offset over all iterations
	Loop *L = LI->getLoopFor(inst->getParent());
	unsigned trip_count = L->getSmallConstantTripCount();
	for (unsigned i=0; i<trip_count; i++) {
	    const SCEV *v_i = SE->getConstant(ConstantInt::get(Type::getInt32Ty(CTX),i));
	    const SCEV *o = addrec->evaluateAtIteration(v_i, *SE);
	    assert(isa<SCEVConstant>(o) &&
		   "Evaluation of SCEV at constant didn't fold correctly?");
	    uint64_t rate = 0;
	    if (streams[s])
		rate = cast<ConstantInt>(streams[s])->getZExtValue();
	    rate = std::max(cast<SCEVConstant>(o)->getValue()->getZExtValue(), rate);
	    streams[s] = ConstantInt::get(Type::getInt32Ty(CTX),rate);
	}
    }
    return streams[s];
}
#undef FAIL

Value *
SKIRKernelInfo::getStreamRate(Value *streams[MAX_STREAMS], unsigned s)
{
    assert(s < MAX_STREAMS);
    
    if (!computed_rates) {
	// iterate over a list of push or pop instructions
	for(inst_list_t::iterator I = pop_list.begin(), E = pop_list.end(); I!=E; ++I) {
	    Instruction *inst = *I;
	    getRate(inst, ins);
	}
	for(inst_list_t::iterator I = push_list.begin(), E = push_list.end(); I!=E; ++I) {
	    Instruction *inst = *I;
	    getRate(inst, outs);
	}
	for(inst_list_t::iterator I = peek_list.begin(), E = peek_list.end(); I!=E; ++I) {
	    Instruction *inst = *I;
	    getRateOffset(inst, peeks);
	}
	computed_rates = true;
    }
    return streams[s];
}

void
SKIRRuntime::addSKIRKernelInfoPass(SKIRRuntimeKernel *k)
{
    k->fpm->add(createIndVarSimplifyPass());
    k->fpm->add(new SKIRKernelInfo(k));
}

void
SKIRRuntime::runSKIRKernelInfoPass(SKIRRuntimeKernel *k)
{
    Function *F = k->work;
    FunctionPassManager PM(F->getParent());
	
    PM.add(new TargetData(F->getParent()));
    PM.add(createIndVarSimplifyPass());
    PM.add(new SKIRKernelInfo(k));

    {
	MutexGuard locked(k->cg->lock);
	PM.run(*F);
    }
}
