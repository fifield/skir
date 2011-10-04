#ifndef _SKIR_RUNTIME_GRAPH_H_
#define _SKIR_RUNTIME_GRAPH_H_

#include <SKIR/SKIRStream.h>
#include "SKIRRuntimeStream.h"
#include "SKIRRuntimeKernel.h"
#include "SKIRScheduler.h"

#include <list>
#include <vector>
#include <map>
#include <iostream>

namespace llvm {

class SKIRRuntime;

class SKIRRuntimeGraph
{
public:
    SKIRRuntimeGraph(SKIRRuntime &runtime);

    ~SKIRRuntimeGraph();

    void setVerbose(bool v);
    SKIRRuntime &getRuntime() { return rt; }

    void callKernel(SKIRRuntimeKernel *kernel);
    void waitKernel(SKIRRuntimeKernel *kernel);
    void pauseKernel(SKIRRuntimeKernel *kernel);
    void unPauseKernel(SKIRRuntimeKernel *kernel);
    void becomeKernel(SKIRRuntimeKernel *kernel, std::vector<SKIRRuntimeKernel*> &kernels);
    void codeGenKernel(SKIRRuntimeKernel *kernel);

    unsigned getNumKernels() { return id2kernel.size(); }

    SKIRRuntimeKernel *getLimiter();

    void selectScheduler(SKIRRuntimeKernel *kernel, SKIRScheduler *sched=0);

    SKIRScheduler *getTbbSched() { return the_tbb_sched; }
    SKIRScheduler *getDPSched() { return the_dp_sched; }
    SKIRScheduler *getOpenCLSched() { return the_opencl_sched; }
    
    SKIRRuntimeKernel* fuseKernels(SKIRRuntimeKernel *kernel0, SKIRRuntimeKernel *kernel1);
    void fissKernel(std::vector<SKIRRuntimeKernel*> &outK, SKIRRuntimeKernel *K, int width);

    void log();
    void dot(std::ostream &output_stream);
    void topo_sort(std::list<SKIRRuntimeKernel *> &output);

private:

    void allocateStreams(SKIRRuntimeKernel *kernel);

    void topo_sort_visit(int k, bool *c, std::list<SKIRRuntimeKernel *>& sorted);

    void addKernel(SKIRRuntimeKernel *k);
    void removeKernel(SKIRRuntimeKernel *k);
    void addEdge(SKIRRuntimeKernel *src, SKIRRuntimeKernel *dst);
    void refreshAdjList(void);
    
    // an adjacency list
    std::vector< std::vector<unsigned> > adj;

    // map kernel ids to kernels
    std::map< unsigned, SKIRRuntimeKernel* > id2kernel;

    SKIRRuntime &rt;
    
    bool verbose;

    SKIRScheduler *the_tbb_sched;
    SKIRScheduler *the_koro_sched;
    SKIRScheduler *the_dp_sched;
    SKIRScheduler *the_opencl_sched;
    SKIRScheduler *the_merge_sched;

    int logfile_id;
    
    SKIRRuntimeKernel *hier_parent;
};

}
#endif //_SKIR_RUNTIME_GRAPH_H_
