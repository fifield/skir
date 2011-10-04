#include "SKIR/SKIRRuntime.h"
#include "SKIRFission.h"
#include "SKIRUtil.h"
//#include <llvm/IntrinsicInst.h>
//#include <llvm/DerivedTypes.h>
//#include <llvm/Instructions.h>
//#include <llvm/Transforms/Utils/Cloning.h>
//#include <llvm/Transforms/Utils/BasicBlockUtils.h>
//#include <llvm/Transforms/Scalar.h>
//#include "inline_stream_ops.h"

using namespace llvm;

// return a newly generated kernel representing the n-way fission K0
//
void SKIRFission::runOnKernel(std::vector<SKIRRuntimeKernel*> &outK,
				   SKIRRuntimeKernel *kernel, int width)
{

}
