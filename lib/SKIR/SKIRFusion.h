#ifndef _SKIR_FUSION_H_
#define _SKIR_FUSION_H_

#include "SKIRRuntimeGraph.h"
#include "SKIRRuntimeStream.h"
#include "SKIRRuntimeKernel.h"

#include "llvm/Function.h"

#include <vector>
#include <set>

namespace llvm {

class SKIRFusion {

public:

    SKIRFusion(SKIRRuntimeGraph &stream_graph) : sg(stream_graph)
    {
    }

    SKIRRuntimeKernel *runOnKernels(SKIRRuntimeKernel &K1, SKIRRuntimeKernel &K2);

private:
    void computeCommonStreams(std::set<SKIRRuntimeStream *> &streams,
			      SKIRRuntimeKernel &K1, SKIRRuntimeKernel &K2);
    void computeOutputStreams(std::vector<SKIRRuntimeStream *> &streams,
			      std::set<SKIRRuntimeStream *> &common, 
			      SKIRRuntimeKernel &K1, SKIRRuntimeKernel &K2);
    void computeInputStreams(std::vector<SKIRRuntimeStream *> &streams,
			     std::set<SKIRRuntimeStream *> &common, 
			     SKIRRuntimeKernel &K1, SKIRRuntimeKernel &K2);

    void replaceStreamOps(std::set<SKIRRuntimeStream*> &common, SKIRRuntimeKernel &k);
    void replaceRenumberedOps(SKIRRuntimeKernel &k,
			      std::vector<Instruction*> &bufs, 
			      std::vector<Instruction*> &idxs);
			      
    void renumberStreamOps(SKIRRuntimeKernel &k, SKIRRuntimeKernel &rtk);
    void cloneWorkFunctionsInto(Function &newWorkFn, Function &F1, Function &F2);

    SKIRRuntimeGraph &sg;
};

}
#endif // _SKIR_FUSION_H_
