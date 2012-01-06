//===----------------------------------------------------------------------===//
// Copyright (c) 2011 Regents of the University of Colorado 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to 
// deal in the Software without restriction, including without limitation the 
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is 
// furnished to do so, subject to the following conditions: 
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software. 
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE. 
//===----------------------------------------------------------------------===//

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

extern void __SKIR_push(skir_stream_idx_t s, skir_stream_element_t e);
extern void __SKIR_pop (skir_stream_idx_t s, skir_stream_element_t e);
extern void __SKIR_peek(skir_stream_idx_t s, skir_stream_element_t e, unsigned int offset);

extern void* __SKIR_kernel(void *workfn, void *args);

extern unsigned long long __SKIR_rdtsc(void);

extern void __SKIR_yield(void *from, void *to);
extern void __SKIR_return(void *from, int r);

extern void __SKIR_become(skir_kernel_ptr_t k);
extern void __SKIR_uncall(skir_kernel_ptr_t k);

#ifdef __cplusplus 
}
#endif

#endif
