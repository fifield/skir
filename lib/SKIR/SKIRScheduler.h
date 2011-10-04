#ifndef _SKIR_SCHEDULER_H_
#define _SKIR_SCHEDULER_H_

#include "SKIRRuntimeKernel.h"

namespace llvm {

class SKIRScheduler {
public:
    virtual void callKernel(SKIRRuntimeKernel *rtk) = 0;
    virtual void removeKernel(SKIRRuntimeKernel *rtk) = 0;
    virtual void waitKernel(SKIRRuntimeKernel *rtk) = 0;
    virtual void pauseKernel(SKIRRuntimeKernel *rtk) = 0;
    virtual void unPauseKernel(SKIRRuntimeKernel *rtk) = 0;

    virtual void runCodeGen(SKIRRuntimeKernel *rtk) = 0;

    virtual void start(void) = 0;
    virtual void stop(void) = 0;

    virtual void setVerbose(bool) = 0;
};

}
#endif
