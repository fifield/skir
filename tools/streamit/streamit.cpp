#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>

#include <SKIR/SKIRStream.h>
#include "skir_intrinsics.h"

#include <sstream>

extern "C" {

/*static*/ void
__streamit_println(int i)
{
    printf("%d\n",i);
}

/*static*/ void
__streamit_println_f(float f)
{
    printf("%f\n",f);
}

/*static*/ void
__streamit_print(char *c)
{
    printf("%s\n",c);
}

typedef struct {
    int num_children;
    int *num_children_ptr;
    void *children[512];
} state_t;

/*static*/ void *noop_init(void *a)
{
    return a;
}

void *
__streamit__pipeline_init(void *s) 
{
    return s;
}

int
__streamit__pipeline_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[]) 
{
    state_t *state = (state_t *)s;
    int n = state->num_children;

    if (n == 0) 
	return 1;
    else if (n == 1) {
	__SKIR_call(state->children[0], (void**)ins, (void**)outs);
    } else {
	skir_kernel_ptr_t k;
	skir_stream_t* tmpi[2] = {0,0};
	skir_stream_t* tmpo[2] = {0,0};
	int i;
	for (i=0; i<n-1; i++) {
	    k = state->children[i];
	    tmpo[0] = (skir_stream_t*)__SKIR_stream(sizeof(int));
	    if (i==0)
		__SKIR_call(k, (void**)ins, (void**)tmpo);
	    else
		__SKIR_call(k, (void**)tmpi, (void**)tmpo);
	    tmpi[0] = tmpo[0];
	}
	k = state->children[i];
	__SKIR_call(k, (void**)tmpi, (void**)outs);
    }
    
    return 1;
}

void *
__streamit__splitjoin_init(void *s)
{
    return s;
}

int
__streamit__splitjoin_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    state_t *state = (state_t *)s;
    int n = state->num_children;

    if (n >= 3) {
	skir_kernel_ptr_t k;
	skir_stream_t* tmp_split[n-1];
	skir_stream_t* tmp_join[n-1];
	skir_stream_t* tmpi[2] = { 0,0 };
	skir_stream_t* tmpo[2] = { 0,0 };
	int i;
	for (i=0; i<n-2; i++) {
	    tmp_split[i] = (skir_stream_t*)__SKIR_stream(sizeof(int));
	    tmp_join[i] = (skir_stream_t*)__SKIR_stream(sizeof(int));
	}
	tmp_split[i] = 0;
	tmp_join[i] = 0;
	k = state->children[0];
	__SKIR_call(k, (void**)ins, (void**)tmp_split);
	for (i=1; i<n-1; i++) {
	    k = state->children[i];
	    tmpi[0] = tmp_split[i-1];
	    tmpo[0] = tmp_join[i-1];
	    __SKIR_call(k, (void**)tmpi, (void**)tmpo);
	}
	k = state->children[i];
	__SKIR_call(k, (void**)tmp_join, (void**)outs);
    }
    return 1;
}

typedef struct {
    int *num_children_ptr;
    int weight;
    void *addr;
} sj_state_t;

typedef struct {
    int num;
    int weights[3];
} sjw_state_t;

} // extern "C"

//
// split duplicate
//

struct split_dup_t {
split_dup_t(int n) : nout(n) {}
    int nout;
};

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
SPLIT_DUP(0)

#undef SPLIT_DUP

#define SPLIT_DUP(DNUM) \
    if ( (num-2) == DNUM ) {			\
	void *k = __SKIR_kernel( (void*)noop_init, (void *)( split_dup_work_##DNUM ), 0 ); \
	__SKIR_call(k, (void **)ins, (void **)outs);			\
	return 1;						\
    }	

extern "C" {

/*static*/ void *
__streamit__splitdupN_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

/*static*/ int
__streamit__splitdupN_work (void *s, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    sj_state_t *state = (sj_state_t*)s;
    int num = *state->num_children_ptr;

    SPLIT_DUP(16);
    SPLIT_DUP(8);
    SPLIT_DUP(6);
    SPLIT_DUP(4);
    SPLIT_DUP(2);

    //fprintf(stderr, "WARNING: Unoptimized split dup (%d)\n", num-2);
    void *k = __SKIR_kernel( (void*)noop_init,
                             (void *)(split_dup_work_0),
                             new split_dup_t(num-2) );
    __SKIR_call(k, (void **)ins, (void **)outs);

    return 1;
}


/*static*/ int
split_dup_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    split_dup_t *s = (split_dup_t*)state;

    int e;
    __SKIR_pop(0, &e);
    for (int i=0; i<s->nout; i++)
	__SKIR_push(i,&e);
    return 0;
}

/*static*/ void *
__streamit__splitdup_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

/*static*/ int
__streamit__splitdup_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    sj_state_t *state = (sj_state_t *)s;
    int n = *state->num_children_ptr;
    int e;
    __SKIR_pop(0, &e);
    for (int i=0; i<n-2; i++) {
	__SKIR_push(i,&e);
    }
    return 0;
}

} // extern "C"

//
// split round-robin
//
struct split_rr_t {
    split_rr_t(int n, int a) : nout(n), arg(a) {}
    int nout;
    int arg;
};

/*static*/ int split_rr_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
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
SPLIT_RR(8,8)

#undef SPLIT_RR
extern "C" {

#define SPLIT_RR(DNUM,DARG) \
    if ( ((num-2) == DNUM ) && (arg == DARG ) ) {			\
	void *k = __SKIR_kernel( (void*)noop_init, (void *)( split_rr_work_##DNUM##_##DARG ), 0 ); \
	__SKIR_call(k, (void **)ins, (void **)outs);			\
	return 1;						\
    }	

void *
__streamit__splitrrN_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

int
__streamit__splitrrN_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    sj_state_t *state = (sj_state_t*)s;
    int num = *state->num_children_ptr;
    int arg = state->weight;

    SPLIT_RR(16,16);
    SPLIT_RR(16,1);
    SPLIT_RR(48,1);
    SPLIT_RR(64,1);
    SPLIT_RR(4,2);
    SPLIT_RR(2,4);
    SPLIT_RR(1,8);
    SPLIT_RR(2,1);
    SPLIT_RR(4,1);
    SPLIT_RR(8,1);
    SPLIT_RR(8,8);

    printf("ERROR UNSUPPORTED split round robin (%d, %d)\n", num-2, arg);
    return 1;
}

#undef SPLIT_RR

#define SPLIT_RR(ARGA, ARGB, ARGC)				\
    static int (* split_rr_work_##ARGA##_##ARGB##_##ARGC )(void*,		\
							   skir_stream_ptr_t*, \
							   skir_stream_ptr_t*) = \
	 &split_rr_work_A_B_C_< ARGA, ARGB, ARGC >;

SPLIT_RR(1, 63, 0)
SPLIT_RR(384, 16, 3)
SPLIT_RR(32,32,0)

#undef SPLIT_RR
#define SPLIT_RR(A, B, C)					\
    if ( (arg0 == A ) && (arg1 == B) && (arg2 == C) ) {			\
	void *k = __SKIR_kernel( (void*)noop_init, (void *)( split_rr_work_##A##_##B##_##C ), 0 ); \
	__SKIR_call(k, (void **)ins, (void **)outs);			\
	return 1;						\
    }	


void *
__streamit__splitrrW_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

int
__streamit__splitrrW_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    sjw_state_t *state = (sjw_state_t*)s;
    int num = state->num;
    int arg0 = state->weights[0];
    int arg1 = state->weights[1];
    int arg2 = state->weights[2];

    //if (num == 3) {

	SPLIT_RR(1, 63, 0)
        SPLIT_RR(384, 16, 3)
	SPLIT_RR(32,32,0)
	    //}

    printf("ERROR UNSUPPORTED split round robin (%d, %d %d %d)\n", num, arg0, arg1, arg2);
    return 1;
}

void *
__streamit__splitrr1_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

int
__streamit__splitrr1_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    sj_state_t *state = (sj_state_t *)s;
    int n = *state->num_children_ptr;
    for (int i=0; i<n-2; i++) {
	for (int j=0; j<state->weight; j++) {
	    int e;
	    __SKIR_pop(0, &e);
	    __SKIR_push(i, &e);
	}
    }
    return 0;
}

/*static*/ void *
__streamit__splitrr0_init(void *a)
{
    return a;
}

/*static*/ int
__streamit__splitrr0_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    return 1;
}

} // extern "C"

//
// join round-robin
//
struct join_rr_t {
    join_rr_t(int n, int a) : nin(n), arg(a) {}
    int nin;
    int arg;
};
/*static*/ int join_rr_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
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

template< int ARGA, int ARGB, int ARGC >
static int join_rr_work_A_B_C_ (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    for (int j=0; j<ARGA; j++) {
	int e;
	__SKIR_pop(0, &e);
	__SKIR_push(0, &e);
    }
    for (int j=0; j<ARGB; j++) {
	int e;
	__SKIR_pop(1, &e);
	__SKIR_push(0, &e);
    }
    for (int j=0; j<ARGC; j++) {
	int e;
	__SKIR_pop(2, &e);
	__SKIR_push(0, &e);
    }
    return 0;
}

template<>
static int join_rr_work_A_B_C_< 0, 0, 0 > (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    return 0;
}

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
JOIN_RR(10,8)
JOIN_RR(6,8)
JOIN_RR(8,8)
JOIN_RR(6,3)
JOIN_RR(2,32)
JOIN_RR(0,0)

#undef JOIN_RR

extern "C" {

#define JOIN_RR(DNUM,DARG) \
    if ( ((num-2) == DNUM ) && (arg == DARG ) ) {			\
	void *k = __SKIR_kernel( (void*)noop_init, (void *)( join_rr_work_##DNUM##_##DARG ), 0 ); \
	__SKIR_call(k, (void **)ins, (void **)outs);			\
	return 1;						\
    }	

/*static*/ void *
__streamit__joinrrN_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

/*static*/ int
__streamit__joinrrN_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    sj_state_t *state = (sj_state_t*)s;
    int num = *state->num_children_ptr;
    int arg = state->weight;
    
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
    JOIN_RR(10,8)
    JOIN_RR(8,8)
    JOIN_RR(6,8)
    JOIN_RR(6,3)
    JOIN_RR(2,32)

    //fprintf(stderr, "WARNING: Unoptimized join round robin (%d, %d)\n", num-2, arg);

    void *k = __SKIR_kernel( (void*)noop_init,
                             (void *)(join_rr_work_0_0),
                             new join_rr_t(num-2, arg));
    __SKIR_call(k, (void **)ins, (void **)outs);

    return 1;
}

#undef JOIN_RR

#define JOIN_RR(ARGA, ARGB, ARGC)				\
    static int (* join_rr_work_##ARGA##_##ARGB##_##ARGC )(void*,		\
							  skir_stream_ptr_t*, \
							  skir_stream_ptr_t*) = \
	 &join_rr_work_A_B_C_< ARGA, ARGB, ARGC >;

JOIN_RR(1,63,0)
JOIN_RR(64,64,0)
JOIN_RR(64,8,3)
JOIN_RR(32,32,0)
JOIN_RR(1,16,0)

#undef JOIN_RR
#define JOIN_RR(A, B, C)					\
    if ( (arg0 == A ) && (arg1 == B) && (arg2 == C) ) {			\
	void *k = __SKIR_kernel( (void*)noop_init, (void *)( join_rr_work_##A##_##B##_##C ), 0 ); \
	__SKIR_call(k, (void **)ins, (void **)outs);			\
	return 1;						\
    }

/*static*/ void *
__streamit__joinrrW_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

/*static*/ int
__streamit__joinrrW_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    sjw_state_t *state = (sjw_state_t*)s;
    int num = state->num;
    int arg0 = state->weights[0];
    int arg1 = state->weights[1];
    int arg2 = state->weights[2];

    JOIN_RR(1,63,0);
    JOIN_RR(64,64,0);
    JOIN_RR(64,8,3);
    JOIN_RR(32,32,0);
    JOIN_RR(1,16,0)

    printf("ERROR UNSUPPORTED join round robin (%d, %d %d %d)\n", num, arg0, arg1, arg2);
    return 1;
}

/*static*/ void *
__streamit__joinrr1_init(void *a)
{
    sj_state_t *state = (sj_state_t*)malloc(sizeof(sj_state_t));
    memcpy(state, a, sizeof(sj_state_t));
    return state;
}

/*static*/ int
__streamit__joinrr1_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    sj_state_t *state = (sj_state_t *)s;
    int n = *state->num_children_ptr;
    for (int i=0; i<n-2; i++) {
	for (int j=0; j<state->weight; j++) {
	    int e;
	    __SKIR_pop(i, &e);
	    __SKIR_push(0, &e);
	}
    }
    return 0;
}

/*static*/ void *
__streamit_Identity_init(void *s)
{
    return s;
}

/*static*/ int
__streamit_Identity_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    int e;
    __SKIR_pop(0, &e);
    __SKIR_push(0, &e);
    return 0;
}

/*static*/ void *
__streamit_FileReaderInt32_init(void *a)
{
    ssize_t *args = (ssize_t *)a;
    char *filename = (char*)args[0];
    int *s = (int*)malloc(sizeof(int)*2);
    s[0] = open(filename,O_RDONLY);
    assert(s[0] > -1 && "FileReaderInt32 file open failed");
    s[1] = 0;
    return (void*)s;
}

/*static*/ int
__streamit_FileReaderInt32_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    int file = ((int*)s)[0];
    int offset = ((int*)s)[1];
    int e;
    ssize_t r = pread(file, &e, sizeof(int), offset);
    //assert(r>-1);
    __SKIR_push(0, &e);
    ((int*)s)[1] = offset + r;
    if (r <= 0) {
	lseek(file, 0, SEEK_SET);
	((int*)s)[1] = 0;
	return 0;
    }
    return (r <= 0);
}

/*static*/ void *
__streamit_FileWriterInt32_init(void *a)
{
    ssize_t *args = (ssize_t *)a;
    char *filename = (char*)args[0];
    int *file = (int*)malloc(sizeof(int));
    *file = open(filename, O_RDWR|O_TRUNC|O_CREAT, S_IRWXU);
    assert(*file > -1);
    return (void*)file;
}

int
__streamit_FileWriterInt32_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    int *file = (int*)s;
    int e;
    __SKIR_pop(0, &e);
    unsigned int r;
    r = write(*file, &e, sizeof(int));
    return 0;
}

/*static*/ void *
__streamit_FileReaderFloat_init(void *a)
{
    ssize_t *args = (ssize_t *)a;
    char *filename = (char*)args[0];
    int *s = (int*)malloc(sizeof(int)*2);
    s[0] = open(filename,O_RDONLY);
    assert(s[0] > -1 && "FileReaderFloat file open failed");
    s[1] = 0;
    return (void*)s;
}

/*static*/ int
__streamit_FileReaderFloat_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    int file = ((int*)s)[0];
    int offset = ((int*)s)[1];
    float e;
    ssize_t r = pread(file, &e, sizeof(float), offset);
    //assert(r>-1);
    __SKIR_push(0, &e);
    ((int*)s)[1] = offset + r;
    //lseek(*file, sizeof(int), SEEK_CUR);
    return (r <= 0);
}

/*static*/ void *
__streamit_FileWriterFloat_init(void *a)
{
    ssize_t *args = (ssize_t *)a;
    char *filename = (char*)args[0];
    int *file = (int*)malloc(sizeof(int));
    *file = open(filename, O_RDWR|O_TRUNC|O_CREAT, S_IRWXU);
    //assert(*file > -1);
    return (void*)file;
}

/*static*/ int
__streamit_FileWriterFloat_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    int *file = (int*)s;
    float e;
    __SKIR_pop(0, &e);
    unsigned int r = write(*file, &e, sizeof(float));
    return 0;
}


static int __dummy_sink = 0;
static unsigned long long __dummy_cnt = 0;
static unsigned long long __dummy_total = 0;
static unsigned long long __dummy_niters = 0;
static double __dummy_t;
static double __dummy_rate_sum = 0.0;

static inline double mysecond()
{
    struct timeval tp;
    //struct timezone tzp;
    int i = gettimeofday(&tp,0);
    return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

/*static*/ void *
__streamit_DummySinkInt_init(void *a)
{
    assert(!__dummy_sink && "There can be only one DummySink");
    __dummy_sink++;
    return a;
}

/*static*/ int
__streamit_DummySinkInt_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    int e;
    __SKIR_pop(0, &e);

    __dummy_cnt++;
    __dummy_total++;

#define DIV 1000000
    static unsigned long long div = DIV;
    if (__dummy_cnt == div) {
	if (__dummy_niters) {
	    double time = mysecond();
	    double elap = time - __dummy_t;
	    if (elap < 0.5) {
		div*=2;
	    }
	    double rate = __dummy_total / elap;
	    printf("%d, %e, %f\n", __dummy_total, rate, elap);
	} else {
	    printf("nsamples, rate, time\n");
	    __dummy_t = mysecond();
	    __dummy_total = 0;
	}
	__dummy_niters++;
	__dummy_cnt = 0;
    }
#undef DIV
    return 0;
}

/*static*/ void *
__streamit_DummySinkFloat_init(void *a)
{
    assert(!__dummy_sink && "There can be only one DummySink");
    __dummy_sink++;
    return a;
}

/*static*/ int
__streamit_DummySinkFloat_work(void *s, skir_stream_t* ins[], skir_stream_t* outs[])
{
    float f;
    __SKIR_pop(0, &f);

    __dummy_cnt++;
    __dummy_total++;

#define DIV 1000000
    static unsigned long long div = DIV;
    if (__dummy_cnt == div) {
	if (__dummy_niters) {
	    double time = mysecond();
	    double elap = time - __dummy_t;
	    if (elap < 0.5) {
		div*=2;
	    }
	    double rate = __dummy_total / elap;
	    printf("%d, %e, %f\n", __dummy_total, rate, elap);
	} else {
	    printf("nsamples, rate, time\n");
	    __dummy_t = mysecond();
	    __dummy_total = 0;
	}
	__dummy_niters++;
	__dummy_cnt = 0;
    }
#undef DIV
    return 0;
}

}
