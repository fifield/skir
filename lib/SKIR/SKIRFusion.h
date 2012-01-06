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
