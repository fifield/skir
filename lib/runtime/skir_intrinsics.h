#ifndef _SKIR_INTRINSICS_H_
#define _SKIR_INTRINSICS_H_

#include <stdlib.h>

#ifdef __cplusplus 
extern "C" {
#endif

#ifndef _SKIR_STREAM_H_
typedef void* skir_stream_ptr_t;
typedef unsigned int skir_stream_idx_t;
typedef void* skir_stream_element_t;
typedef void* skir_kernel_ptr_t;
#endif

extern void __SKIR_call(void *s, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[]);
extern void __SKIR_wait(void *s);
extern skir_stream_ptr_t __SKIR_stream(int size);
extern skir_stream_ptr_t __SKIR_array(void *begin, void *end, \
				      unsigned int size, unsigned int stride);

extern void __SKIR_push(skir_stream_idx_t s, skir_stream_element_t e);
extern void __SKIR_pop (skir_stream_idx_t s, skir_stream_element_t e);
extern void __SKIR_peek(skir_stream_idx_t s, skir_stream_element_t e, unsigned int offset);

extern void* __SKIR_kernel(void *initfn, void *workfn, void *args);

extern unsigned long long __SKIR_rdtsc(void);

extern void __SKIR_yield(void *from, void *to);
extern void __SKIR_return(void *from, int r);

extern void __SKIR_become(skir_kernel_ptr_t k);

#ifdef __cplusplus 
}
#endif

#endif
