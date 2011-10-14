

#include "inline_stream_ops.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#ifndef min
#define min(a,b) ( ((a)<(b)) ? (a):(b) )
#endif

//
// generic work functions
//
extern "C" {

void *
__SKIRRT_workfn(skir_rt_state_t   *rt_state, 
		void              *kernel_state, 
		skir_stream_t *ins[],
		skir_stream_t *outs[])
{
    return (void*)1;
}

void *
__SKIRRT_workfn_skel(skir_rt_state_t   *rt_state, 
		     void              *kernel_state, 
		     skir_stream_t *ins[],
		     skir_stream_t *outs[])
{
    START_TSC(rt_state->cycles);

    void *v = __SKIRRT_workfn_extern(rt_state, kernel_state, ins, outs);

    rt_state->niter++;
    GET_TSC(rt_state->cycles);

    return v;
}

//
// fusion support
//

void *
__SKIRRT_workfn_inline2(/*skir_rt_state_t *rt_state, */
			void            *kernel_state,
			skir_stream_t *ins[],
			skir_stream_t *outs[])
{
    inline_inline_t *s = (inline_inline_t *)kernel_state;

    //START_TSC(rt_state->cycles);

    while (s->iter0 < s->niter0) {
	skir_kernel_ptr_t k = __SKIRRT_workfn_extern0(s->rt_state0, s->state0, s->ins0, s->outs0);
	if (k != 0) return k;
	s->iter0++;
    }
    while (s->iter1 < s->niter1) {
	skir_kernel_ptr_t k = __SKIRRT_workfn_extern1(s->rt_state1, s->state1, s->ins1, s->outs1);
	if (k != 0) return k;
	s->iter1++;
    }
    s->iter0 = 0;
    s->iter1 = 0;

    // for (i=0; !k && i<s->niter0; i++)
    // 	k = __SKIRRT_workfn_extern0(s->rt_state0, s->state0, s->ins0, s->outs0);

    // for (i=0; !k && i<s->niter1; i++)
    // 	k = __SKIRRT_workfn_extern1(s->rt_state1, s->state1, s->ins1, s->outs1);

    //rt_state->niter++;
    //GET_TSC(rt_state->cycles);

    return 0;
}

void *
__SKIRRT_workfn_inline2_1_1(/*skir_rt_state_t *rt_state, */
			    void            *kernel_state,
			    skir_stream_t *ins[],
			    skir_stream_t *outs[])
{
    inline_inline_t *s = (inline_inline_t *)kernel_state;
    skir_kernel_ptr_t k = 0;
    //int i;

    //START_TSC(rt_state->cycles);

    //if (s->iter0 == 0) {
	k = __SKIRRT_workfn_extern0(s->rt_state0, s->state0, s->ins0, s->outs0);
	if (k != 0) return k;
	//s->iter0 = 1;
	//}
	//if (s->iter1 == 0) {
	k = __SKIRRT_workfn_extern1(s->rt_state1, s->state1, s->ins1, s->outs1);
	if (k != 0) return k;
	//s->iter1 = 1;
    //}
    //s->iter0 = 0;
    //s->iter1 = 0;

    //rt_state->niter++;
    //GET_TSC(rt_state->cycles);

    return 0;
}

void 
__SKIRRT_inline_push_fuse_4 (void *buf, int *idx, skir_stream_element_t e)
{
    int i = *idx;
    unsigned char *b = (unsigned char *)buf;
    memcpy(&b[i], e, 4);
    *idx = i + 4;
}

void 
__SKIRRT_inline_pop_fuse_4 (void *buf, int *idx, skir_stream_element_t e)
{
    int i = *idx;
    unsigned char *b = (unsigned char *)buf;
    memcpy(e, &b[i], 4);
    *idx = i + 4;
}

void 
__SKIRRT_inline_peek_fuse_4 (void *buf, int *idx, skir_stream_element_t e)
{
    assert("fused peek?" && 0);
}

} // extern "C"

//
// standard stream operations
//

// inline push
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_push_E_B_ (skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];

    size_t head = s->head;
    size_t next = (head + ELMSZ) % BUFSZ;
    while (next == s->tail) {
	__SKIRRT_would_block(s->src, s->dst);
    }
    memcpy(&s->buf[head], e, ELMSZ);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

template<> void
__SKIRRT_inline_push_E_B_< 0, 0 > (skir_stream_t *p[],skir_stream_idx_t idx,skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];

    size_t head = s->head;
    size_t next = (head + s->elem_size) % STREAM_BUFFER_SIZE;
    while (next == s->tail) {
	__SKIRRT_would_block(s->src, s->dst);
    }
    memcpy(&s->buf[head], e, s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

void (* __SKIRRT_inline_push)(skir_stream_t **, skir_stream_idx_t, skir_stream_element_t) = \
    &__SKIRRT_inline_push_E_B_< 0, 0 >;

#define __SKIRRT_INLINE_PUSH(ELEMENT_SIZE, BUFFER_SIZE) \
    void (* __SKIRRT_inline_push_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **,\
								 skir_stream_idx_t, \
								 skir_stream_element_t) = \
	&__SKIRRT_inline_push_E_B_< ELEMENT_SIZE, BUFFER_SIZE >

__SKIRRT_INLINE_PUSH(4,32768);
__SKIRRT_INLINE_PUSH(4,128);
__SKIRRT_INLINE_PUSH(8,128);
__SKIRRT_INLINE_PUSH(8,32768);

// inline pop
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_pop_E_B_ (skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t tail = s->tail;
    while (s->head == tail) {
	__SKIRRT_would_block(s->dst, s->src);
    }
    memcpy(e, &s->buf[tail], ELMSZ);
    s->tail = (tail + ELMSZ) % BUFSZ;
}

template<> void
__SKIRRT_inline_pop_E_B_< 0, 0 > (skir_stream_t *p[],skir_stream_idx_t idx,skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t tail = s->tail;
    while (s->head == tail) {
	__SKIRRT_would_block(s->dst, s->src);
    }
    memcpy(e, &s->buf[tail],  s->elem_size);
    s->tail = (tail + s->elem_size) % STREAM_BUFFER_SIZE;
}

void (* __SKIRRT_inline_pop)(skir_stream_t **, skir_stream_idx_t, skir_stream_element_t) = \
    &__SKIRRT_inline_pop_E_B_< 0, 0 >;

#define __SKIRRT_INLINE_POP(ELEMENT_SIZE, BUFFER_SIZE) \
    void (* __SKIRRT_inline_pop_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **,\
								skir_stream_idx_t, \
								skir_stream_element_t) = \
	&__SKIRRT_inline_pop_E_B_< ELEMENT_SIZE, BUFFER_SIZE >

__SKIRRT_INLINE_POP(4,32768);
__SKIRRT_INLINE_POP(4,128);
__SKIRRT_INLINE_POP(8,128);
__SKIRRT_INLINE_POP(8,32768);

// inline peek
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_peek_E_B_ (skir_stream_t *p[], skir_stream_idx_t idx, 
				skir_stream_element_t e, uint32_t o)
{
    skir_stream_t *s = p[idx];

    size_t tail = s->tail;
    size_t head = s->head;
    size_t offset = o;
    offset = (tail + (offset * ELMSZ));
    while (!( ((tail < head) && (offset < head)) ||
	      ((tail > head) && (offset < (head+BUFSZ))) )) {
	__SKIRRT_would_block(s->dst, s->src);
	head = s->head;
    }
    memcpy(e, &s->buf[offset%BUFSZ], ELMSZ);
}

template<> void
__SKIRRT_inline_peek_E_B_< 0, 0 > (skir_stream_t *p[],skir_stream_idx_t idx,
				   skir_stream_element_t e, uint32_t o)
{
    skir_stream_t *s = p[idx];

    size_t tail = s->tail;
    size_t head = s->head;
    size_t offset = o;
    offset = (tail + (offset * s->elem_size));
    while (!( ((tail < head) && (offset < head)) ||
	      ((tail > head) && (offset < (head+STREAM_BUFFER_SIZE))) )) {
	__SKIRRT_would_block(s->dst, s->src);
	head = s->head;
    }
    memcpy(e, &s->buf[offset%STREAM_BUFFER_SIZE], s->elem_size);
}

void (* __SKIRRT_inline_peek)(skir_stream_t **,skir_stream_idx_t,skir_stream_element_t,uint32_t) = \
    &__SKIRRT_inline_peek_E_B_< 0, 0 >;

#define __SKIRRT_INLINE_PEEK(ELEMENT_SIZE, BUFFER_SIZE) \
    void (* __SKIRRT_inline_peek_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **,\
								 skir_stream_idx_t, \
								 skir_stream_element_t,uint32_t) = \
	&__SKIRRT_inline_peek_E_B_< ELEMENT_SIZE, BUFFER_SIZE >

__SKIRRT_INLINE_PEEK(4,32768);
__SKIRRT_INLINE_PEEK(8,32768);
__SKIRRT_INLINE_PEEK(4,128);
__SKIRRT_INLINE_PEEK(8,128);

//
// blocking stream operations
//

#define __SKIRRT_PASS_RETRY 2
#define __SKIRRT_PASS(A) if (A) (A)--; else usleep(1);

void
__SKIRRT_inline_push_block(skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];

    size_t head = s->head;
    size_t next = (head + s->elem_size) % STREAM_BUFFER_SIZE;
    size_t tail = s->tail;
    size_t retry = __SKIRRT_PASS_RETRY;

    while (next == tail) {
	__SKIRRT_PASS(retry);
	tail = s->tail;
    }

    memcpy(&s->buf[head], e,  s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

void
__SKIRRT_inline_pop_block(skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t tail = s->tail;
    size_t head = s->head;
    size_t retry = __SKIRRT_PASS_RETRY;

    while (head == tail) {
	__SKIRRT_PASS(retry);
	head = s->head;
    }
    memcpy(e, &s->buf[tail],  s->elem_size);
    s->tail = (tail + s->elem_size) % STREAM_BUFFER_SIZE;
}

void
__SKIRRT_inline_peek_block(skir_stream_t *p[], skir_stream_idx_t idx, 
			   skir_stream_element_t e, uint32_t o)
{
    skir_stream_t *s = p[idx];

    size_t tail = s->tail;
    size_t head = s->head;
    size_t offset = o;
    size_t retry = __SKIRRT_PASS_RETRY;

    offset = (tail + (offset * s->elem_size));
    while ( !( ((tail < head) && (offset < head)) ||
	       ((tail > head) && (offset < (head+STREAM_BUFFER_SIZE))) ) ) {
	__SKIRRT_PASS(retry);
	head = s->head;
    }
    memcpy(e, &s->buf[offset%STREAM_BUFFER_SIZE], s->elem_size);
}

//
// rate/space computation routines
//

static inline size_t
__SKIRRT_inline_pop_space(skir_stream_t *s)
{
    long long num = s->head - s->tail;
    if (num < 0)
	num += STREAM_BUFFER_SIZE;
    return num;
}

static inline size_t
__SKIRRT_inline_push_space(skir_stream_t *s)
{
    long long next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
    long long num = s->tail - next;
    if (num < 0)
	num += STREAM_BUFFER_SIZE;
    return num;
}

size_t
__SKIRRT_inline_compute_niters(void **v, 
			       skir_stream_t *ins[], int nins,
			       skir_stream_t *outs[], int nouts)
{
    int i;
    int n = STREAM_BUFFER_SIZE;

    for (i=0; i<nouts; i++) {
	skir_stream_t *s = outs[i];
	size_t space = (__SKIRRT_inline_push_space(s)) / s->elem_size;
	long long npush = space / s->push_rate;
	if (npush <= 0) { *v = s->dst; return 0; }
	n = min(n,npush);
    }

    for (i=0; i<nins; i++) {
	skir_stream_t *s = ins[i];
	size_t space = (__SKIRRT_inline_pop_space(s)) / s->elem_size;
	long long npop = (space - s->peek_rate) / s->pop_rate;
	if (npop <= 0) { *v = s->src; return 0; }

	n = min(n,npop);
    }

    *v = 0;
    return n;
}

static inline size_t
__SKIRRT_inline_compute_niters_1_1(void **v, 
				   skir_stream_t *ins[], int nins,
				   skir_stream_t *outs[], int nouts)
{
    skir_stream_t *in = ins[0];
    skir_stream_t *out = outs[0];

    long long next = (out->head + out->elem_size) % STREAM_BUFFER_SIZE;
    long long outspace = out->tail - next;
    if (outspace < 0)
	outspace += STREAM_BUFFER_SIZE;
    if (outspace <= 0) {
	*v = out->dst;
	return 0;
    }
    size_t npush = (size_t)outspace / out->elem_size;;

    long long inspace = in->head - in->tail;
    if (inspace < 0)
	inspace += STREAM_BUFFER_SIZE;
    if (inspace <= 0) {
	*v = in->src;
	return 0;
    }
    size_t npop = (size_t)inspace / out->elem_size;

    size_t n = min(npop,npush);

    *v = 0;
    return n;
}
//} // extern "C"

template< int ELMSZ, int BUFSZ >
static inline size_t __SKIRRT_inline_pop_space_E_B_(skir_stream_t *s)
{
    long long num = s->head - s->tail;
    if (num < 0)
	num += BUFSZ;
    return num;
}

template< int ELMSZ, int BUFSZ >
static inline size_t __SKIRRT_inline_push_space_E_B_(skir_stream_t *s)
{
    long long next = (s->head + ELMSZ) % BUFSZ;
    long long num = s->tail - next;
    if (num < 0)
	num += BUFSZ;
    return num;
}

template< int ELMSZ, int BUFSZ >
size_t __SKIRRT_inline_compute_niters_E_B_(void **v, 
					   skir_stream_t *ins[], int nins,
					   skir_stream_t *outs[], int nouts)
{
    int i;
    int n = BUFSZ;

    for (i=0; i<nouts; i++) {
	skir_stream_t *s = outs[i];
	size_t space = (__SKIRRT_inline_push_space_E_B_<ELMSZ,BUFSZ>(s)) / ELMSZ;
	long long npush = space / s->push_rate;
	if (npush <= 0) { *v = s->dst; return 0; }
	n = min(n,npush);
    }

    for (i=0; i<nins; i++) {
	skir_stream_t *s = ins[i];
	size_t space = (__SKIRRT_inline_pop_space_E_B_<ELMSZ,BUFSZ>(s)) / ELMSZ;
	long long npop = (space - s->peek_rate) / s->pop_rate;
	if (npop <= 0) { *v = s->src; return 0; }

	n = min(n,npop);
    }

    *v = 0;
    return n;
}

extern "C"
void * __SKIRRT_workfn_loop(skir_rt_state_t *rt_state, void *kernel_state,
			    skir_stream_t *ins[], skir_stream_t *outs[])
{
    void *k = 0;
    while (k == 0) {
	rt_state->niter++;
	k = __SKIRRT_workfn_extern(rt_state, kernel_state, ins, outs);
    }
    return k;
}
//
// task based scheduler skeleton work functions
//

template< int ELMSZ, int BUFSZ >
void * __SKIRRT_workfn_nocheck_E_B_(skir_rt_state_t *rt_state, void *kernel_state,
				    skir_stream_t *ins[], skir_stream_t *outs[])
{
    void *v;
    size_t niter = __SKIRRT_inline_compute_niters_E_B_<ELMSZ,BUFSZ>(&v, ins, 1, outs, 1);
    if (v) return v;

    // start timing
    START_TSC(rt_state->cycles);

    void *k = 0;
    while (niter--) {
	rt_state->niter++;
	k = __SKIRRT_workfn_extern(rt_state, kernel_state, ins, outs);
	if (k) break;
    }

    // stop timing
    GET_TSC(rt_state->cycles);

    return k;
}

template<>
void * __SKIRRT_workfn_nocheck_E_B_< 0, 0 >(skir_rt_state_t *rt_state, void *kernel_state,
					    skir_stream_t *ins[], skir_stream_t *outs[])
{
    void *v;
    size_t niter = __SKIRRT_inline_compute_niters(&v, ins, 1, outs, 1);
    if (v) return v;

    // start timing
    START_TSC(rt_state->cycles);

    void *k = 0;
    while (niter--) {
	rt_state->niter++;
	k = __SKIRRT_workfn_extern(rt_state, kernel_state, ins, outs);
	if (k) break;
    }

    // stop timing
    GET_TSC(rt_state->cycles);

    return k;

}

void *(* __SKIRRT_workfn_nocheck)(skir_rt_state_t*, void*, skir_stream_t**, skir_stream_t**) = \
    &__SKIRRT_workfn_nocheck_E_B_< 0, 0>;

#define __SKIRRT_WORKFN_NOCHECK(ELEMENT_SIZE, BUFFER_SIZE) \
    void* (* __SKIRRT_workfn_nocheck_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_rt_state_t*, void*, \
								     skir_stream_t**, \
								     skir_stream_t**)= \
	&__SKIRRT_workfn_nocheck_E_B_< ELEMENT_SIZE, BUFFER_SIZE >

__SKIRRT_WORKFN_NOCHECK(4,128);
__SKIRRT_WORKFN_NOCHECK(8,128);
__SKIRRT_WORKFN_NOCHECK(0,16384);
__SKIRRT_WORKFN_NOCHECK(0,32768);
__SKIRRT_WORKFN_NOCHECK(4,16384);
__SKIRRT_WORKFN_NOCHECK(4,32768);
__SKIRRT_WORKFN_NOCHECK(8,16384);
__SKIRRT_WORKFN_NOCHECK(8,32768);
__SKIRRT_WORKFN_NOCHECK(16,16384);
__SKIRRT_WORKFN_NOCHECK(16,32768);
__SKIRRT_WORKFN_NOCHECK(32,16384);
__SKIRRT_WORKFN_NOCHECK(32,32768);

// optimized work function skel.
extern "C" void *
__SKIRRT_workfn_nocheck_1_1(skir_rt_state_t   *rt_state, 
			    void              *kernel_state,
			    skir_stream_t *ins[],
			    skir_stream_t *outs[])
{
    size_t i;
    void *v, *k;

    skir_stream_t in, out;
    skir_stream_t *ip[1] = { &in };
    skir_stream_t *op[1] = { &out };
    volatile size_t *real_head = &(outs[0]->head);
    volatile size_t *real_tail = &(ins[0]->tail);

    //    i = __SKIRRT_inline_compute_niters_E_B_<4,16384>(&v, ins, 1, outs, 1);
    i = __SKIRRT_inline_compute_niters_E_B_<4,32768>(&v, ins, 1, outs, 1);
    //i = __SKIRRT_inline_compute_niters_E_B_<4,128>(&v, ins, 1, outs, 1);
    if (v) return v;

    in.tail = *real_tail;
    in.inp = ins[0]->inp;

    out.head = *real_head;
    out.outp = outs[0]->outp;

    ip[0] = &in;
    op[0] = &out;

    // start timing
    START_TSC(rt_state->cycles);

    k = 0;
    while (i) {
	rt_state->niter++;
	i--;
	k = __SKIRRT_workfn_extern(rt_state, kernel_state, ip, op);
	*real_tail = in.tail;
	*real_head = out.head;
	if (k) break;
    }

    // stop timing
    GET_TSC(rt_state->cycles);

    return k;
}

// optimized work function skel.
void *
__SKIRRT_workfn_array(skir_rt_state_t   *rt_state, 
		      void              *kernel_state,
		      skir_stream_t *ins[],
		      skir_stream_t *outs[])
{
    size_t i;
    void *k = 0;

    skir_stream_t in, out;
    skir_stream_t *ip[1] = { &in };
    skir_stream_t *op[1] = { &out };

    in.stride = ins[0]->stride;
    in.elem_size = in.stride;
    in.tail = (size_t)(ins[0]->inp);

    out.stride = outs[0]->stride;
    out.elem_size = out.stride;
    out.head = (size_t)(outs[0]->outp);

    START_TSC(rt_state->cycles);

    i = rt_state->niter;
    rt_state->niter = 0;

    while (i) {
	i--;
	rt_state->niter++;
	k = __SKIRRT_workfn_extern(rt_state, kernel_state, ip, op);
	if (k) break;
    }

    GET_TSC(rt_state->cycles);

    return k;
}

//
// split join data parallelism support
//

void __SKIRRT_inline_push_nocheck_s(skir_stream_t *p, skir_stream_element_t);
void __SKIRRT_inline_pop_nocheck_s(skir_stream_t *p, skir_stream_element_t);

extern "C" {

void *
__SKIRRT_dp_split_work(dp_splitter_work_t *state,
		       skir_stream_t     *ins[],
		       skir_stream_t     *outs[])
{
    void *v = 0;
    while (!v) {
    size_t niter = __SKIRRT_inline_compute_niters(&v, ins, 1, outs, state->num_streams);
    if (v) return v;
	
    while (niter--) {
	int i,j;
	for (i=0; i<state->num_streams; i++) {
	    int rate = state->rate[i];
	    for (j=0; j<rate; j++) {
		float f;
		__SKIRRT_inline_pop_nocheck_s(ins[0], &f);
		__SKIRRT_inline_push_nocheck_s(outs[i], &f);
		//e = __SKIRRT_inline_pop_nocheck_int32(ins[0]);
		//__SKIRRT_inline_push_nocheck_int32(outs[i], e);
	    }
	}
    }
    }
    return 0;

}

void *
__SKIRRT_dp_join_work(dp_splitter_work_t *state,
		      skir_stream_t     *ins[],
		      skir_stream_t     *outs[])
{
    void *v = 0;
    while (!v) {
	size_t niter = __SKIRRT_inline_compute_niters(&v, ins, state->num_streams, outs, 1);
	if (v) return v;

	while (niter--) {
	    int i,j;
	    for (i=0; i<state->num_streams; i++) {
		int rate = state->rate[i];
		for (j=0; j<rate; j++) {
		    float f;
		    __SKIRRT_inline_pop_nocheck_s(ins[i], &f);
		    __SKIRRT_inline_push_nocheck_s(outs[0], &f);
		    //		    e = __SKIRRT_inline_pop_nocheck_int32(ins[i]);
		    //		    __SKIRRT_inline_push_nocheck_int32(outs[0], e);
		}
	    }
	}
    }
    return 0;
}

} // extern "C"

//
// pop nocheck variations 
//

// standard version
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_pop_nocheck_E_B_(skir_stream_t *p[], skir_stream_idx_t idx, 
				      skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t tail = s->tail;
    memcpy(e, &s->buf[tail],  ELMSZ);
    s->tail = (tail + ELMSZ) % BUFSZ;
}

template<> 
void __SKIRRT_inline_pop_nocheck_E_B_< 0, 0 > (skir_stream_t *p[], skir_stream_idx_t idx,
					       skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t tail = s->tail;
    memcpy(e, &s->buf[tail],  s->elem_size);
    s->tail = (tail + s->elem_size) % STREAM_BUFFER_SIZE;
}

void (* __SKIRRT_inline_pop_nocheck)(skir_stream_t **, skir_stream_idx_t, skir_stream_element_t) = \
    &__SKIRRT_inline_pop_nocheck_E_B_< 0, 0 >;

// inp version
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_pop_nocheck_p_E_B_(skir_stream_t *p[], skir_stream_idx_t idx, 
					skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t tail = s->tail;
    memcpy(e, &s->inp[tail],  ELMSZ);
    s->tail = (tail + ELMSZ) % BUFSZ;
}

template<> 
void __SKIRRT_inline_pop_nocheck_p_E_B_< 0, 0 > (skir_stream_t *p[], skir_stream_idx_t idx,
						 skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t tail = s->tail;
    memcpy(e, &s->inp[tail],  s->elem_size);
    s->tail = (tail + s->elem_size) % STREAM_BUFFER_SIZE;
}

void (* __SKIRRT_inline_pop_nocheck_p)(skir_stream_t **,skir_stream_idx_t,skir_stream_element_t) = \
    &__SKIRRT_inline_pop_nocheck_p_E_B_< 0, 0 >;

// generate definitions
// 
#define __SKIRRT_INLINE_POP_NOCHECK(ELEMENT_SIZE, BUFFER_SIZE) \
    void (* __SKIRRT_inline_pop_nocheck_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **,\
									skir_stream_idx_t, \
									skir_stream_element_t) = \
	&__SKIRRT_inline_pop_nocheck_E_B_< ELEMENT_SIZE, BUFFER_SIZE >; \
    void (* __SKIRRT_inline_pop_nocheck_p_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **,\
									  skir_stream_idx_t, \
									  skir_stream_element_t) = \
	&__SKIRRT_inline_pop_nocheck_p_E_B_< ELEMENT_SIZE, BUFFER_SIZE >
    
__SKIRRT_INLINE_POP_NOCHECK(4,128);
__SKIRRT_INLINE_POP_NOCHECK(8,128);
__SKIRRT_INLINE_POP_NOCHECK(4,16384);
__SKIRRT_INLINE_POP_NOCHECK(4,32768);
__SKIRRT_INLINE_POP_NOCHECK(8,16384);
__SKIRRT_INLINE_POP_NOCHECK(8,32768);
__SKIRRT_INLINE_POP_NOCHECK(16,16384);
__SKIRRT_INLINE_POP_NOCHECK(16,32768);
__SKIRRT_INLINE_POP_NOCHECK(32,16384);
__SKIRRT_INLINE_POP_NOCHECK(32,32768);

void
__SKIRRT_inline_pop_nocheck_s(skir_stream_t *s, skir_stream_element_t e)
{
    memcpy(e, &s->buf[s->tail],  s->elem_size);
    s->tail = (s->tail + s->elem_size) % STREAM_BUFFER_SIZE;
}

void
__SKIRRT_inline_pop_nocheck_i(size_t i, skir_stream_element_t e)
{
    skir_stream_t *s = (skir_stream_t *)i;
    memcpy(e, &s->buf[s->tail],  s->elem_size);
    s->tail = (s->tail + s->elem_size) % STREAM_BUFFER_SIZE;
}
int
__SKIRRT_inline_pop_nocheck_i_int32(size_t i)
{
    skir_stream_t *s = (skir_stream_t *)i;
    int e;
    memcpy(&e, &s->buf[s->tail], sizeof(e));
    //size_t tail = (s->tail + 64) % STREAM_BUFFER_SIZE;
    s->tail = (s->tail + sizeof(e)) % STREAM_BUFFER_SIZE;
    //__SKIR_prefetch(&s->buf[tail], 0, 0);
    return e;
}
int
__SKIRRT_inline_pop_nocheck_int32(skir_stream_t *p)
{
    skir_stream_t *s = p;
    int e;
    //assert(s->head != s->tail);
    memcpy(&e, &s->buf[s->tail], sizeof(e));
    //size_t tail = (s->tail + 64) % STREAM_BUFFER_SIZE;
    s->tail = (s->tail + sizeof(e)) % STREAM_BUFFER_SIZE;
    //__SKIR_prefetch(&s->buf[tail], 0, 0);
    return e;
}
float
__SKIRRT_inline_pop_nocheck_i_float(size_t i)
{
    skir_stream_t *s = (skir_stream_t *)i;
    float e;
    memcpy(&e, &s->buf[s->tail],  sizeof(e));
    s->tail = (s->tail + sizeof(e)) % STREAM_BUFFER_SIZE;
    return e;
}

//
// peek nocheck variations
//

// standard 
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_peek_nocheck_E_B_(skir_stream_t *p[], skir_stream_idx_t idx,
				       skir_stream_element_t e, uint32_t offset)
{
    skir_stream_t *s = p[idx];
    size_t tail = (s->tail + (offset*ELMSZ)) % BUFSZ;
    memcpy(e, &s->buf[tail], ELMSZ);
}

template<> 
void __SKIRRT_inline_peek_nocheck_E_B_< 0, 0 > (skir_stream_t *p[], skir_stream_idx_t idx,
						skir_stream_element_t e, uint32_t offset)

{
    skir_stream_t *s = p[idx];
    size_t tail = (s->tail + (offset*s->elem_size)) % STREAM_BUFFER_SIZE;
    memcpy(e, &s->buf[tail], s->elem_size);
}

void (* __SKIRRT_inline_peek_nocheck)(skir_stream_t **,skir_stream_idx_t, \
				      skir_stream_element_t, uint32_t) =	\
    &__SKIRRT_inline_peek_nocheck_E_B_< 0, 0 >;

// inp
// 
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_peek_nocheck_p_E_B_(skir_stream_t *p[], skir_stream_idx_t idx,
					 skir_stream_element_t e, uint32_t offset)
{
    skir_stream_t *s = p[idx];
    size_t tail = (s->tail + (offset*ELMSZ)) % BUFSZ;
    memcpy(e, &s->inp[tail], ELMSZ);
}

template<> 
void __SKIRRT_inline_peek_nocheck_p_E_B_< 0, 0 > (skir_stream_t *p[], skir_stream_idx_t idx,
						  skir_stream_element_t e, uint32_t offset)

{
    skir_stream_t *s = p[idx];
    size_t tail = (s->tail + (offset*s->elem_size)) % STREAM_BUFFER_SIZE;
    memcpy(e, &s->inp[tail], s->elem_size);
}

void (* __SKIRRT_inline_peek_nocheck_p)(skir_stream_t **,skir_stream_idx_t, \
					skir_stream_element_t, uint32_t) = \
    &__SKIRRT_inline_peek_nocheck_p_E_B_< 0, 0 >;

// generate
//
#define __SKIRRT_INLINE_PEEK_NOCHECK(ELEMENT_SIZE, BUFFER_SIZE) \
    void (* __SKIRRT_inline_peek_nocheck_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **,\
									 skir_stream_idx_t, \
									 skir_stream_element_t, \
									 uint32_t) = \
	&__SKIRRT_inline_peek_nocheck_E_B_< ELEMENT_SIZE, BUFFER_SIZE >; \
    void (* __SKIRRT_inline_peek_nocheck_p_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **, \
									   skir_stream_idx_t, \
									   skir_stream_element_t, \
									   uint32_t) = \
	&__SKIRRT_inline_peek_nocheck_p_E_B_< ELEMENT_SIZE, BUFFER_SIZE >

__SKIRRT_INLINE_PEEK_NOCHECK(4,128);
__SKIRRT_INLINE_PEEK_NOCHECK(8,128);
__SKIRRT_INLINE_PEEK_NOCHECK(4,16384);
__SKIRRT_INLINE_PEEK_NOCHECK(4,32768);
__SKIRRT_INLINE_PEEK_NOCHECK(8,16384);
__SKIRRT_INLINE_PEEK_NOCHECK(8,32768);
__SKIRRT_INLINE_PEEK_NOCHECK(16,16384);
__SKIRRT_INLINE_PEEK_NOCHECK(16,32768);
__SKIRRT_INLINE_PEEK_NOCHECK(32,16384);
__SKIRRT_INLINE_PEEK_NOCHECK(32,32768);

void
__SKIRRT_inline_peek_nocheck_s(skir_stream_t *p,skir_stream_element_t e, uint32_t offset)
{
    skir_stream_t *s = p;
    size_t tail = (s->tail + (offset*s->elem_size)) % STREAM_BUFFER_SIZE;
    memcpy(e, &s->buf[tail],  s->elem_size);
}
void
__SKIRRT_inline_peek_nocheck_i(size_t i,skir_stream_element_t e, uint32_t offset)
{
    skir_stream_t *s = (skir_stream_t *)i;
    size_t tail = (s->tail + (offset*s->elem_size)) % STREAM_BUFFER_SIZE;
    memcpy(e, &s->buf[tail],  s->elem_size);
}
int
__SKIRRT_inline_peek_nocheck_i_int32(size_t i, uint32_t offset)
{
    skir_stream_t *s = (skir_stream_t *)i;
    int e;
    size_t n = (s->tail + (offset*sizeof(e))) % STREAM_BUFFER_SIZE;
    memcpy(&e, &s->buf[n],  sizeof(e));
    return e;
}
float
__SKIRRT_inline_peek_nocheck_i_float(size_t i, uint32_t offset)
{
    skir_stream_t *s = (skir_stream_t *)i;
    float e;
    size_t n = (s->tail + (offset*sizeof(e))) % STREAM_BUFFER_SIZE;
    memcpy(&e, &s->buf[n],  sizeof(e));
    return e;
}

//
// push nocheck variations
//

// standard
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_push_nocheck_E_B_(skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t head = s->head;
    size_t next = (head + ELMSZ) % BUFSZ;
    memcpy(&s->buf[head], e, ELMSZ);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

template<> 
void __SKIRRT_inline_push_nocheck_E_B_< 0, 0 > (skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t head = s->head;
    size_t next = (head + s->elem_size) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[head], e,  s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

void (* __SKIRRT_inline_push_nocheck)(skir_stream_t **,skir_stream_idx_t,skir_stream_element_t) = \
    &__SKIRRT_inline_push_nocheck_E_B_< 0, 0 >;

// outp
//
template< int ELMSZ, int BUFSZ >
void __SKIRRT_inline_push_nocheck_p_E_B_(skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t head = s->head;
    size_t next = (head + ELMSZ) % BUFSZ;
    memcpy(&s->outp[head], e, ELMSZ);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

template<> 
void __SKIRRT_inline_push_nocheck_p_E_B_< 0, 0 > (skir_stream_t *p[], skir_stream_idx_t idx, skir_stream_element_t e)
{
    skir_stream_t *s = p[idx];
    size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
    memcpy(&s->outp[s->head], e,  s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

void (* __SKIRRT_inline_push_nocheck_p)(skir_stream_t **,
					skir_stream_idx_t,		\
					skir_stream_element_t) =	\
    &__SKIRRT_inline_push_nocheck_p_E_B_< 0, 0 >;

// generate
//
#define __SKIRRT_INLINE_PUSH_NOCHECK(ELEMENT_SIZE, BUFFER_SIZE) \
    void (* __SKIRRT_inline_push_nocheck_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **, \
									 skir_stream_idx_t, \
									 skir_stream_element_t)= \
	&__SKIRRT_inline_push_nocheck_E_B_< ELEMENT_SIZE, BUFFER_SIZE >; \
    void (* __SKIRRT_inline_push_nocheck_p_##ELEMENT_SIZE##_##BUFFER_SIZE)(skir_stream_t **, \
									   skir_stream_idx_t, \
									   skir_stream_element_t)=\
	&__SKIRRT_inline_push_nocheck_p_E_B_< ELEMENT_SIZE, BUFFER_SIZE >

__SKIRRT_INLINE_PUSH_NOCHECK(4,128);
__SKIRRT_INLINE_PUSH_NOCHECK(8,128);
__SKIRRT_INLINE_PUSH_NOCHECK(4,16384);
__SKIRRT_INLINE_PUSH_NOCHECK(4,32768);
__SKIRRT_INLINE_PUSH_NOCHECK(8,16384);
__SKIRRT_INLINE_PUSH_NOCHECK(8,32768);
__SKIRRT_INLINE_PUSH_NOCHECK(16,16384);
__SKIRRT_INLINE_PUSH_NOCHECK(16,32768);
__SKIRRT_INLINE_PUSH_NOCHECK(32,16384);
__SKIRRT_INLINE_PUSH_NOCHECK(32,32768);


void
__SKIRRT_inline_push_nocheck_s(skir_stream_t *p, skir_stream_element_t e)
{
    skir_stream_t *s = p;
    size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[s->head], e,  s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
}
void
__SKIRRT_inline_push_nocheck_i(size_t i, skir_stream_element_t e)
{
    skir_stream_t *s = (skir_stream_t *)i;
    size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[s->head], e, s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
}
void
__SKIRRT_inline_push_nocheck_i_int32(size_t i, int32_t e)
{
    skir_stream_t *s = (skir_stream_t *)i;
    size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
    //assert(next != s->tail);
    //size_t head = (s->head + 64) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[s->head], &e, s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
    //__SKIR_prefetch(&s->buf[head], 1, 0);
}
void
__SKIRRT_inline_push_nocheck_int32(skir_stream_t *p, int32_t e)
{
    skir_stream_t *s = p;
    size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
    //size_t head = (s->head + 64) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[s->head], &e, s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
    //__SKIR_prefetch(&s->buf[head], 1, 0);
}
void
__SKIRRT_inline_push_nocheck_i_float(size_t i, float e)
{
    skir_stream_t *s = (skir_stream_t *)i;
    size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[s->head], &e, s->elem_size);
    s->head = next;
    RECORD_PUSH(s->num_push);
}

void
__SKIRRT_inline_poke_nocheck(skir_stream_t *p[], skir_stream_idx_t idx, 
			     skir_stream_element_t e, uint32_t offset)
{
    skir_stream_t *s = p[idx];
    size_t head = (s->head + (offset*s->elem_size)) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[head], e, s->elem_size);
}
void
__SKIRRT_inline_poke_nocheck_s(skir_stream_t *p,skir_stream_element_t e, uint32_t offset)
{
    skir_stream_t *s = p;
    size_t head = (s->head + (offset*s->elem_size)) % STREAM_BUFFER_SIZE;
    memcpy(&s->buf[head], e, s->elem_size);
}

void
__SKIRRT_inline_array_nocheck_s(skir_stream_t *p, skir_stream_element_t e, uint32_t offset)
{
    char *a = (char*)p;
    memcpy(e, a+(offset*4), 4);
}

void
__SKIRRT_inline_array_push_nocheck(skir_stream_t *p[], skir_stream_idx_t idx, 
				   skir_stream_element_t e)
{
    skir_array_stream_t *s = (skir_array_stream_t *)p[idx];
    memcpy((void*)s->head, e, s->elem_size);
    s->head += s->stride;
    RECORD_PUSH(s->num_push);
}

void
__SKIRRT_inline_array_pop_nocheck(skir_stream_t *p[], skir_stream_idx_t idx, 
				  skir_stream_element_t e)
{
    skir_array_stream_t *s = (skir_array_stream_t *)p[idx];
    memcpy(e, (void*)s->tail, s->elem_size);
    s->tail += s->stride;
}

//
// socket streams
//
#if 0

#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct {
    int server_sock;
    int client_sock;
    unsigned short port;
} __SKIRRT_socket_source_t;

void *
__SKIRRT_socket_source_init(void *a) 
{
    size_t arg = (size_t)a;
    __SKIRRT_socket_source_t *state =
	(__SKIRRT_socket_source_t*)malloc(sizeof(__SKIRRT_socket_source_t));
    state->server_sock = 0;
    state->client_sock = 0;
    state->port = (unsigned short)(arg & 0xffff);

    struct sockaddr_in laddr;  //local address
    laddr.sin_family = AF_INET;
    laddr.sin_addr.s_addr = htonl(INADDR_ANY);
    laddr.sin_port = htons(state->port);

    /* Create socket for incoming connections */
    if ((state->server_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket() failed:");
	state->server_sock = 0;
	return (void*)state;
    }

    /* Bind to the local address */
    if (bind(state->server_sock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
	perror("bind() failed:");
	state->server_sock = 0;
	return (void*)state;
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(state->server_sock, 5) < 0) {
	perror("listen() failed:");
	state->server_sock = 0;
	return (void*)state;
    }

    //printf("%p\n", state); fflush(0);
    return (void*)state;
}

int
__SKIRRT_socket_source_work(void *kernel_state, 
			    skir_stream_t *ins[],
			    skir_stream_t *outs[])
{
    __SKIRRT_socket_source_t *state = (__SKIRRT_socket_source_t *)kernel_state;
    skir_stream_t *s = outs[0];
    size_t next = (s->head + s->elem_size) % STREAM_BUFFER_SIZE;

    if (next == s->tail)
	__SKIRRT_would_block(s->src, s->dst);

    if (!state->server_sock)
	return 1;

    else if (!state->client_sock) {
	struct sockaddr_in caddr;  // client address
	socklen_t len = sizeof(caddr);

	//printf("waiting...\n"); fflush(0);

	// waiting for a client
	if ((state->client_sock = accept(state->server_sock, (struct sockaddr *) &caddr, &len))<0) {
	    perror("accept() failed\n");
	    state->client_sock = 0;
	    return 0;
	}

	//printf("connected!\n"); fflush(0);
    }

    else {
	// connected to a client
	int npush;
	if (s->head >= s->tail)
	    npush = (STREAM_BUFFER_SIZE - s->head);
	else
	    npush = (s->tail - s->head);

	if ((npush = recv(state->client_sock, &(s->buf[s->head]), npush, 0)) <= 0) {
	    close(state->client_sock);
	    state->client_sock = 0;
	    return 1;
	}
	else {
	    //printf("npush: %d\n",npush);
	    next = (s->head + npush) % STREAM_BUFFER_SIZE;
	    s->head = next;
	}
    }

    return 0;
}

typedef struct {
    int sock;
    unsigned int addr;
    unsigned short port;
} __SKIRRT_socket_sink_t;

void *
__SKIRRT_socket_sink_init(void *a)
{
    unsigned long long arg = (unsigned long long)a;
    __SKIRRT_socket_sink_t *state
	= (__SKIRRT_socket_sink_t *)malloc(sizeof(__SKIRRT_socket_sink_t));
    state->sock = 0;
    state->port = (unsigned short)(arg & 0xffff);
    state->addr = (unsigned int)(arg >> 32);
    //printf("%p\n", state); fflush(0);
    return (void*)state;
}

int
__SKIRRT_socket_sink_work(void *kernel_state, 
			  skir_stream_t *ins[],
			  skir_stream_t *outs[])
{
    __SKIRRT_socket_sink_t *state = (__SKIRRT_socket_sink_t *)kernel_state;
    //printf("%p\n", state); fflush(0);
    skir_stream_t *s = ins[0];
    if (s->head == s->tail) {
	if (s->src == (void*)1) {
	    close(state->sock);
	    state->sock = 0;
	}
	__SKIRRT_would_block(s->dst, s->src);
    }

    if (!state->sock) {
	if ((state->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
	    perror("socket() failed");
	    state->sock = 0;
	    return 1;
	}

	struct sockaddr_in saddr;  // server address
	saddr.sin_family      = AF_INET;
	saddr.sin_addr.s_addr = htonl(state->addr);
	saddr.sin_port        = htons(state->port);

	//printf("%s\n", inet_ntoa(saddr.sin_addr));

	if (connect(state->sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
	    perror("connect() failed");
	    state->sock = 0;
	    return 0;
	}

    }

    int npop;
    if (s->head > s->tail)
	npop = (s->head - s->tail);
    else
	npop = (STREAM_BUFFER_SIZE - s->tail);

    if ((npop = send(state->sock, &(s->buf[s->tail]), npop, 0)) <= 0) {
	if (npop < 0) perror("send() error");
	close(state->sock);
	state->sock = 0;
	return 1;
    }

    //printf("npop: %d\n",npop);
    s->tail = (s->tail + npop) % STREAM_BUFFER_SIZE;
    return 0;
}

// end of socket streams
#endif
