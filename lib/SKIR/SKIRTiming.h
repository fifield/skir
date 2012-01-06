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

#ifndef __SKIR_TIMING_H__
#define __SKIR_TIMING_H__

#include <sys/time.h>

//
// from time-warp-test.c found in an interweb tube
//
#define u64 unsigned long long

#ifdef __x86_64__
#define DECLARE_ARGS(val, low, high)    unsigned int low, high, val=0
#define EAX_EDX_VAL(val, low, high)     (((u64)low) | (((u64)high) << 32))
#define EAX_EDX_ARGS(val, low, high)    "a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high)     "=a" (low), "=d" (high)
#else
#define DECLARE_ARGS(val, low, high)    unsigned long long val
#define EAX_EDX_VAL(val, low, high)     (val)
#define EAX_EDX_ARGS(val, low, high)    "A" (val)
#define EAX_EDX_RET(val, low, high)     "=A" (val)
#endif

static inline unsigned long long __rdtscll(void)
{
	DECLARE_ARGS(val, low, high);
	asm volatile("cpuid"
                        : "=a" (val)   /* EAX into b (output) */
                        : "a"  (val)   /* a into EAX (input)  */
                        :"%ebx","%ecx","%edx"); /* cpuid always clobbers these */
	asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));

	return EAX_EDX_VAL(val, low, high);
}

#define rdtscll(val) do { (val) = __rdtscll(); } while (0)

#define rdtod(val)					\
do {							\
	struct timeval tv;				\
							\
	gettimeofday(&tv, NULL);			\
	(val) = tv.tv_sec * 1000000ULL + tv.tv_usec;	\
} while (0)

#endif// __SKIR_TIMING_H__
