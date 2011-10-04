#ifndef _SKIR_INLINE_OPS_H_
#define _SKIR_INLINE_OPS_H_

#include <SKIR/SKIRStream.h>

#ifdef __cplusplus 
extern "C" {
#endif

extern unsigned long long __SKIR_rdtsc(void);

//
// profiling macros
//

//#define RECORD_PUSH(cnt) {  }
#define RECORD_PUSH(cnt)  cnt++

#define START_TSC(tsc) { }
#define GET_TSC(tsc) { }
//#define START_TSC(tsc)  (tsc = __SKIR_rdtsc())
//#define GET_TSC(tsc)  (tsc = (__SKIR_rdtsc() - tsc))

//
// 
//

//#define __SKIR_prefetch(a, b, c) __builtin_prefetch((a),(b),(c))
//extern void __SKIR_prefetch(void *, int, int);

/* dummy call indicating blocking condition */
extern void __SKIRRT_would_block(void*, void*);

/* dummy call for obtaining prototype info from llvm */
extern void *__SKIRRT_workfn_extern(skir_rt_state_t   *rt_state, 
				    void              *kernel_state, 
				    skir_stream_t *ins[],
				    skir_stream_t *outs[]);
extern void *__SKIRRT_workfn_extern0(skir_rt_state_t   *rt_state, 
				    void              *kernel_state, 
				    skir_stream_t *ins[],
				    skir_stream_t *outs[]);
extern void *__SKIRRT_workfn_extern1(skir_rt_state_t   *rt_state, 
				    void              *kernel_state, 
				    skir_stream_t *ins[],
				    skir_stream_t *outs[]);


extern void (* __SKIRRT_inline_pop_nocheck)(skir_stream_t *p[], skir_stream_idx_t idx,
					    skir_stream_element_t e);
extern void (*__SKIRRT_inline_push_nocheck)(skir_stream_t *p[], skir_stream_idx_t idx,
					    skir_stream_element_t e);
extern void (*__SKIRRT_inline_peek_nocheck)(skir_stream_t *p[], skir_stream_idx_t idx,
					    skir_stream_element_t e, uint32_t offset);

extern void __SKIRRT_inline_pop_block(skir_stream_t *p[], skir_stream_idx_t idx,
					skir_stream_element_t e);
extern void __SKIRRT_inline_push_block(skir_stream_t *p[], skir_stream_idx_t idx,
					 skir_stream_element_t e);
extern void __SKIRRT_inline_peek_block(skir_stream_t *p[], skir_stream_idx_t idx,
					 skir_stream_element_t e, uint32_t offset);

extern size_t
__SKIRRT_inline_compute_niters(void **v,
			       skir_stream_t* ins[], int nins,
			       skir_stream_t* outs[], int nouts);

typedef struct {
    int iter0;
    int niter0;
    skir_rt_state_t *rt_state0;
    void *state0;
    skir_stream_t **ins0;
    skir_stream_t **outs0;
    int iter1;
    int niter1;
    skir_rt_state_t *rt_state1;
    void *state1;
    skir_stream_t **ins1;
    skir_stream_t **outs1;
} inline_inline_t;

typedef struct {
    int next_stream;
    int niter;
    int *rate;
    int num_streams;
} dp_splitter_work_t;

#ifdef __cplusplus 
}
#endif

#endif
