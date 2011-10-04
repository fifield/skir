#ifndef _SKIR_STREAM_H_
#define _SKIR_STREAM_H_

#include <inttypes.h>
#include <stdlib.h>

//
// this is the only header shared between skir and inline_stream_ops.c
//
#ifdef __cplusplus 
extern "C" {
#endif

typedef void* skir_stream_element_t;
typedef unsigned int skir_stream_idx_t;
typedef void* skir_kernel_ptr_t;
typedef void* skir_stream_ptr_t;

// runtime internal kernel state
typedef struct {
    void *stk;
    size_t niter;
    unsigned long long cycles;
} skir_rt_state_t;

// define the various stream implementation data structures
// see inline_stream_ops.cpp for implementation details

#define CACHE_LINE_SIZE 64
#define NUM_STREAM_HEADERS 0
#define STREAM_BUFFER_SIZE (1024*32)

// STREAM
typedef struct {
    // state
    unsigned id;
    unsigned elem_size;
    unsigned stride;
    unsigned dst_dead;
    unsigned src_dead;
    
    // note these can be different than the
    // rates in the corresponding SKIRRuntimeStream
    // because of e.g. kernel fusion
    int pop_rate;
    int push_rate;
    int peek_rate;

    void *src;        // SKIRRuntimeKernel*
    void *dst;        // SKIRRuntimeKernel*
    void *rs;         // SKIRRuntimeStream*

    char _pad0[CACHE_LINE_SIZE-5*sizeof(unsigned)-3*sizeof(int)-3*sizeof(void*)];

    // cache line 1
    /*volatile*/ size_t head;
    char *outp;
    unsigned long long num_push;
    size_t next_tail;
    char _pad1[CACHE_LINE_SIZE-3*sizeof(size_t)-sizeof(unsigned long long)];

    // cache line 2
    /*volatile*/ size_t tail;
    char *inp;
    size_t next_head;
    char _pad2[CACHE_LINE_SIZE-3*sizeof(size_t)];

    // buffer
    //char *buf;
    //char buf[STREAM_BUFFER_SIZE] __attribute__ ((aligned (CACHE_LINE_SIZE)));
    char buf[0];

}  __attribute__ ((aligned (CACHE_LINE_SIZE))) skir_stream_t;

// ARRAY
typedef skir_stream_t skir_array_stream_t;

#ifdef __cplusplus 
}
#endif

#endif
