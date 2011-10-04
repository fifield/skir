#ifndef _SKIR_FISSION_H_
#define _SKIR_FISSION_H_

#include "SKIRRuntimeGraph.h"
#include "SKIRRuntimeKernel.h"

#include <vector>

namespace llvm {

class SKIRFission {

public:

    SKIRFission(SKIRRuntimeGraph &stream_graph) : sg(stream_graph)
    {
    }

    void runOnKernel(std::vector<SKIRRuntimeKernel*> &outK, SKIRRuntimeKernel *kernel, int width);

private:

    SKIRRuntimeGraph &sg;
};

}
#endif //  _SKIR_FISSION_H_
