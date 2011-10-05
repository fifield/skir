#include <llvm/Pass.h>

#include "SKIRRuntimeKernel.h"
#include <vector>

using namespace llvm;

struct SKIRKernelInfo : public FunctionPass
{
public:
    typedef std::vector<Instruction *> inst_list_t;

    static char ID;
    SKIRKernelInfo() : FunctionPass((intptr_t)&ID) { init(NULL); }
    SKIRKernelInfo(SKIRRuntimeKernel *k): FunctionPass((intptr_t)&ID) { init(k); }
    
    void getAnalysisUsage(AnalysisUsage &AU) const;

    bool runOnFunction(Function &work);

    //void print(raw_ostream &O, const Module*);

    Value *getPushRate(unsigned stream) { return getStreamRate(outs, stream); }
    Value *getPopRate(unsigned stream)  { return getStreamRate(ins, stream); }
    Value *getPeekRate(unsigned stream) { return getStreamRate(peeks, stream); }

    // get a list of all push/pop instructions in kernel known to have index idx
    //void getPops(unsigned idx, inst_list_t &list) { return findByIdx(idx, pop_list, list); }
    //void getPushs(unsigned idx, inst_list_t &list) { return findByIdx(idx, push_list, list); }

    // returns number of input/output streams, or -1 if 
    // the number cannot be compile-time determined
    int getNumIns(void);
    int getNumOuts(void);

private:
    void init(SKIRRuntimeKernel *k);

    Value *getStreamRate(Value *streams[], unsigned s);

    Value *getRate(Instruction *inst, Value *streams[]);
    Value *getRateOffset(Instruction *inst, Value *streams[]);

    //void findByIdx(unsigned idx, inst_list_t &input_list, inst_list_t &output_list);

    bool computed_rates;
    bool tryKnownRates(Function &work);

    inst_list_t pop_list;
    inst_list_t push_list;
    inst_list_t peek_list;

    static const unsigned MAX_STREAMS = 512;
    Value *ins[MAX_STREAMS];
    Value *outs[MAX_STREAMS];
    Value *peeks[MAX_STREAMS];
    Value *peeks_rate[MAX_STREAMS];
    
    int nins;
    int nouts;

    bool is_fixed_rate;

    SKIRRuntimeKernel *kernel;
};


