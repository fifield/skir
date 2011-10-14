
#ifndef _SIMPLE_KERNELS_H_
#define _SIMPLE_KERNELS_H_

#include "skir_intrinsics.h"

#include <iostream>
#include <math.h>
#include <sys/time.h>

//
// do nothing init fn
//
static void *noop_init(void *a)
{
    return a;
}

//
// convert int to float
//
static int int_to_float_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    int e;
    __SKIR_pop(0, &e);
    float f = (float)e;
    __SKIR_push(0, &f);
    return 0;
}

//
// convert float to int
//
static int float_to_int_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    float f;
    __SKIR_pop(0, &f);
    int e = floorf(f + 0.5);
    __SKIR_push(0, &e);
    return 0;
}

//
// simple source
//
static int simple_0_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    int *i = (int *)state;

    __SKIR_push(0, i);

    (*i)--;
    
    if ((*i)<=0) 
	return 1;
    else
	return 0;
}

//
// split round-robin
//
struct split_rr_t {
    split_rr_t(int n, int a) : nout(n), arg(a) {}
    int nout;
    int arg;
};
static int split_rr_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    split_rr_t *s = (split_rr_t*)state;
    for (int i=0; i<s->nout; i++) {
	for (int j=0; j<s->arg; j++) {
	    int e;
	    __SKIR_pop(0, &e);
	    __SKIR_push(i, &e);
	}
    }
    return 0;
}


template< int NOUT, int ARG >
static int split_rr_work_N_A_ (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    for (int i=0; i<NOUT; i++) {
	for (int j=0; j<ARG; j++) {
	    int e;
	    __SKIR_pop(0, &e);
	    __SKIR_push(i, &e);
	}
    }
    return 0;
}

template<>
static int split_rr_work_N_A_< 0, 0 > (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    split_rr_t *s = (split_rr_t*)state;
    for (int i=0; i<s->nout; i++) {
	for (int j=0; j<s->arg; j++) {
	    int e;
	    __SKIR_pop(0, &e);
	    __SKIR_push(i, &e);
	}
    }
    return 0;
}

#define SPLIT_RR(DNUM,DARG) \
    static int (* split_rr_work_##DNUM##_##DARG )(void*,skir_stream_ptr_t*,skir_stream_ptr_t*) = \
	 &split_rr_work_N_A_< DNUM, DARG >;

template< int ARGA, int ARGB, int ARGC >
static int split_rr_work_A_B_C_ (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    for (int j=0; j<ARGA; j++) {
	int e;
	__SKIR_pop(0, &e);
	__SKIR_push(0, &e);
    }
    for (int j=0; j<ARGB; j++) {
	int e;
	__SKIR_pop(0, &e);
	__SKIR_push(1, &e);
    }
    for (int j=0; j<ARGC; j++) {
	int e;
	__SKIR_pop(0, &e);
	__SKIR_push(2, &e);
    }
    return 0;
}

template<>
static int split_rr_work_A_B_C_< 0, 0, 0 > (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    return 0;
}

#define SPLIT_RR_3ARG(ARGA, ARGB, ARGC)				\
    static int (* split_rr_work_##ARGA##_##ARGB##_##ARGC )(void*,		\
							   skir_stream_ptr_t*, \
							   skir_stream_ptr_t*) = \
	 &split_rr_work_A_B_C_< ARGA, ARGB, ARGC >;

SPLIT_RR(16,16)
SPLIT_RR(16,1)
SPLIT_RR(48,1)
SPLIT_RR(64,1)
SPLIT_RR(4,2)
SPLIT_RR(2,4)
SPLIT_RR(1,8)
SPLIT_RR(2,1)
SPLIT_RR(4,1)
SPLIT_RR(8,1)

//
// split duplicate
//
struct split_dup_t {
split_dup_t(int n) : nout(n) {}
    int nout;
};
int
split_dup_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    split_dup_t *s = (split_dup_t*)state;

    int e;
    __SKIR_pop(0, &e);
    for (int i=0; i<s->nout; i++)
	__SKIR_push(i,&e);
    return 0;
}

template< int NOUT > 
static int split_dup_work_N_ (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    split_dup_t *s = (split_dup_t*)state;

    int e;
    __SKIR_pop(0, &e);
    for (int i=0; i<NOUT; i++)
	__SKIR_push(i,&e);
    return 0;
}

template<>
static int split_dup_work_N_< 0 > (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    split_dup_t *s = (split_dup_t*)state;

    int e;
    __SKIR_pop(0, &e);
    for (int i=0; i<s->nout; i++)
	__SKIR_push(i,&e);
    return 0;
}

#define SPLIT_DUP(DNUM) \
    static int (* split_dup_work_##DNUM )(void*,skir_stream_ptr_t*,skir_stream_ptr_t*) = \
	 &split_dup_work_N_< DNUM >;

SPLIT_DUP(16)
SPLIT_DUP(8)
SPLIT_DUP(6)
SPLIT_DUP(4)
SPLIT_DUP(2)

//
// join round-robin
//
struct join_rr_t {
    join_rr_t(int n, int a) : nin(n), arg(a) {}
    int nin;
    int arg;
};
static int join_rr_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
     join_rr_t *s = (join_rr_t*)state;
    
     for (int i=0; i<s->nin; i++) {
	 for (int j=0; j<s->arg; j++) {
	     int e;
	     __SKIR_pop(i, &e);
	     __SKIR_push(0, &e);
	 }
     }
    return 0;
}

template< int NIN, int ARG >
static int join_rr_work_N_A_ (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    for (int i=0; i<NIN; i++) {
	 for (int j=0; j<ARG; j++) {
	     int e;
	     __SKIR_pop(i, &e);
	     __SKIR_push(0, &e);
	 }
     }
    return 0;
}

template<>
static int join_rr_work_N_A_< 0, 0> (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    join_rr_t *s = (join_rr_t*)state;
    
    for (int i=0; i<s->nin; i++) {
	for (int j=0; j<s->arg; j++) {
	    int e;
	    __SKIR_pop(i, &e);
	    __SKIR_push(0, &e);
	}
    }
    return 0;
}

#define JOIN_RR(DNUM, DARG) \
    static int (* join_rr_work_##DNUM##_##DARG )(void *,skir_stream_ptr_t*,skir_stream_ptr_t*) = \
	&join_rr_work_N_A_<DNUM,DARG>;

JOIN_RR(16,1)
JOIN_RR(16,16)
JOIN_RR(32,32)
JOIN_RR(6,1)
JOIN_RR(8,1)
JOIN_RR(12,2)
JOIN_RR(4,1)
JOIN_RR(2,1)
JOIN_RR(1,8)
JOIN_RR(2,4)
JOIN_RR(4,2)
JOIN_RR(48,1)
JOIN_RR(64,1)

static int joiner_rr_work_2_1_16 (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    int e;
    __SKIR_pop(0, &e);
    __SKIR_push(0, &e);
    
    for (int j=0; j<16; j++) {
	int f;
	__SKIR_pop(1, &f);
	__SKIR_push(0, &f);
    }
    return 0;
}

//
// simple sink
//
struct simple_1_0_t {
    simple_1_0_t() 
	: total(0),
	  cnt(0),
	  iters(0),
	  rate_sum(0.0),
#define TEN_MILLION 10000000
	  div(TEN_MILLION)
    {
	time = 0;
	total=0;
    }
    unsigned long long cnt;
    double time;

    unsigned long long iters;
    double rate_sum;

    unsigned long long total;

    unsigned int div;
    unsigned int scratch;
};

static double mysecond()
{
    struct timeval tp;
    int i = gettimeofday(&tp,0);
    return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

static int simple_1_0_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    int e = 0;
    simple_1_0_t *s = (simple_1_0_t *)state;

    __SKIR_pop(0, &e);

    s->cnt++;
    s->total++;
    if (s->cnt == s->div) {
	if (s->iters) {
	    double time = mysecond();
	    double elap = time-s->time;
	    //double rate = s->total/elap;
	    double rate = s->cnt/elap;
	    //s->rate_sum += rate;
	    //std::cout << rate << "," << s->rate_sum/s->iters << "\n";
	    std::cout << s->total << ", " << rate << ", " << elap << "\n";
	} else {
	    std::cout << "nsamples, rate, time\n";
	    //s->time = mysecond();
	    s->total = 0;
	}
	s->iters++;
	s->cnt = 0;
	s->time = mysecond();
    }
    return 0;
}


//
// int printer
// 
struct int_printer_t {
    int_printer_t(char *p) : prefix(p) {}
    char *prefix;
};

static int int_printer_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    int_printer_t *s = (int_printer_t *)state;
    int e;
    
    __SKIR_pop(0, &e);
    
    std::cout << s->prefix << e << "\n";
    return 0;
}

//
// float printer
// 
struct float_printer_t {
    float_printer_t(char *p) : prefix(p) {}
    char *prefix;
};

static int float_printer_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    float_printer_t *s = (float_printer_t *)state;
    float e = 0;
    
    __SKIR_pop(0, &e);
    
    printf("%f\n", e);
    //std::cout << s->prefix << e << "\n";
    return 0;
}

#endif
