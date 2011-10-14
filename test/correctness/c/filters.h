
#ifndef _FILTERS_H_
#define _FILTERS_H_

#include "skir_intrinsics.h"

#define NUM_TAPS 1024

struct fir_state_t {
    fir_state_t(int n) {
	for (int i=0; i<NUM_TAPS; i++) {
	    taps[i] = 1;
	    history[i] = 0;
	}
	history_pos = 0;
	ntaps = n;
    }
    int taps[NUM_TAPS];
    int history[NUM_TAPS];
    int history_pos;
    int ntaps;
};

static int
fir_1_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;

    __SKIR_pop(0, &s->history[s->history_pos]);
    
    int sum = 0;
    for (int i=0; i<s->ntaps; i++) 
	sum += (s->history[(s->history_pos+i)%s->ntaps] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%s->ntaps;
    
    __SKIR_push(0, &sum);

    return 0;
}

static int
fir1024_1_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;

    __SKIR_pop(0, &s->history[s->history_pos]);
    
    int sum = 0;
    for (int i=0; i<1024; i++) 
	sum += (s->history[(s->history_pos+i)%1024] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%1024;
    
    __SKIR_push(0, &sum);

    return 0;
}

static int
fir128_1_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;

    __SKIR_pop(0, &s->history[s->history_pos]);
    
    int sum = 0;
    for (int i=0; i<128; i++) 
	sum += (s->history[(s->history_pos+i)%128] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%128;
    
    __SKIR_push(0, &sum);

    return 0;
}

static int
fir256_1_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;

    __SKIR_pop(0, &s->history[s->history_pos]);
    
    int sum = 0;
    for (int i=0; i<256; i++) 
	sum += (s->history[(s->history_pos+i)%256] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%256;
    
    __SKIR_push(0, &sum);

    return 0;
}


static int
fir512_1_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;

    __SKIR_pop(0, &s->history[s->history_pos]);
    
    int sum = 0;
    for (int i=0; i<512; i++) 
	sum += (s->history[(s->history_pos+i)%512] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%512;
    
    __SKIR_push(0, &sum);

    return 0;
}


static int
fir1024_2_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;

    int a, b;

    __SKIR_pop(0, &a);
    __SKIR_pop(1, &b);
    s->history[s->history_pos] = a + b;

    int sum = 0;
    for (int i=0; i<1024; i++) 
	sum += (s->history[(s->history_pos+i)%1024] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%1024;

    __SKIR_push(0, &sum);

    return 0;
}

static int
fir_2_1_work (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;

    int a, b;
    __SKIR_pop(0, &a);
    __SKIR_pop(1, &b);
    s->history[s->history_pos] = a + b;

    int sum = 0;
    for (int i=0; i<s->ntaps; i++) 
	sum += (s->history[(s->history_pos+i)%s->ntaps] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%s->ntaps;

    __SKIR_push(0, &sum);

    return 0;
}


#if 0
static skir_kernel_ptr_t 
fir_1_1_work_transform (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;
    //skir_stream_ptr_t in = ins[0];
    //skir_stream_ptr_t out = outs[0];
    
    int sum;

    switch(s->entry) {
    default: goto entry0;
    case 0:  goto entry0;
    case 1:  goto entry1;
    }
 
 entry0:
    sum = 0;
    if (skir_kernel_ptr_t k = __SKIR_pop(0, &s->history[s->history_pos]))
	return k;
    s->entry++;
    
    //s->history[s->history_pos] = e;
    for (int i=0; i<s->ntaps; i++) 
	sum += (s->history[(s->history_pos+i)%s->ntaps] * s->taps[i]);
    s->history_pos = (s->history_pos+1)%s->ntaps;
    
 entry1:
    if (skir_kernel_ptr_t k = __SKIR_push(0, &sum))
	return k;
    s->entry=0;

    return NULL;
}
#endif

#if 0
template <int NTAPS> skir_kernel_ptr_t 
fir_1_1_work_nocheck (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;
    skir_stream_ptr_t in = ins[0];
    skir_stream_ptr_t out = outs[0];
    
    while (1) {
	int npop = pop_space(in);
	if (!npop) return in->src;

	int npush = push_space(out);
	if (!npush) return out->dst;
	
	int n = std::min(npush, npop);
	//std::cout << n << "\n";
	
	//if (!n) return fir_1_1_work(state, ins, outs);
     
	while (n--) {
	    pop_nocheck(in, &s->history[s->history_pos]);
	    
	    int sum = 0;
	    for (int i=0; i<NTAPS; i++) 
		sum += (s->history[(s->history_pos+i)%NTAPS] * s->taps[i]);
	    s->history_pos = (s->history_pos+1)%NTAPS;
	    
	    push_nocheck(out, &sum);
	}
    }
    //s->entry = 0;
}
#endif

#if 0
static skir_kernel_ptr_t 
fir_2_1_work_nocheck (void *state, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{
    fir_state_t *s = (fir_state_t*)state;
    skir_stream_ptr_t in0 = ins[0];
    skir_stream_ptr_t in1 = ins[1];
    skir_stream_ptr_t out = outs[0];
    
    while (1) {
	int npop0 = pop_space(in0);
	if (!npop0) return in0->src;
	
	int npop1 = pop_space(in1);
	if (!npop1) return in1->src;
	
	int npop = std::min(npop0, npop1);
	
	int npush = push_space(out);
	if (!npush) return out->dst;
	
	int n = std::min(npush, npop);
	//std::cout << n << "\n";
	
	//if (!n) return fir_1_1_work(state, ins, outs);
	
	while (n--) {
	    int e0, e1;
	    pop_nocheck(in0, &e0);
	    pop_nocheck(in1, &e1);
	    s->history[s->history_pos] = e0 + e1;
	    
	    int sum = 0;
	    for (int i=0; i<s->ntaps; i++) 
		sum += (s->history[(s->history_pos+i)%s->ntaps] * s->taps[i]);
	    s->history_pos = (s->history_pos+1)%s->ntaps;
	    
	    push_nocheck(out, &sum);
	}
    }
}
#endif

#endif
