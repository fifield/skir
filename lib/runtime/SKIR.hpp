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

#include <vector>
#include <assert.h>

namespace skir {

extern "C" {
#include "skir_intrinsics.h"
}

typedef skir_stream_element_t StreamElement;
typedef skir_stream_ptr_t     StreamPtr;
typedef skir_kernel_ptr_t     KernelPtr;

typedef StreamPtr* InputStreams;
typedef StreamPtr* OutputStreams;

class StreamBase
{
private:
    StreamPtr stream;
    int offset;

public:
    StreamBase(int size) { 
	stream = __SKIR_stream(size);
    }

    StreamBase(StreamPtr *, int i) : offset(i) {}

    inline int getOffset() { return offset; }
    inline StreamPtr getStream() { return stream; }
};

typedef std::vector<StreamBase*> StreamVector;

template <class T> class Stream : public StreamBase
{

public:
    // XXX I think these restrictions can be lifted?
    Stream() : StreamBase(sizeof(T)) {
	assert(sizeof(T) <= sizeof(StreamElement) && "try using pointers instead");
    }

    Stream(StreamPtr *p, int i) : StreamBase(p,i) {
	assert(sizeof(T) <= sizeof(StreamElement) && "try using pointers instead");
    }

    inline void push(T t) {
	void *p = &t;
	__SKIR_push(getOffset(), p);
    }

    inline T pop() {
	T t;
	void *p = &t;
	__SKIR_pop(getOffset(), p);
	return t;
    }
};

template <class D> class Kernel
{

private:
    KernelPtr kernel;

public:

    // call a kernel
    void operator()(const StreamVector& ins, const StreamVector& outs)
    {
	unsigned int i;

	StreamPtr* _ins = new StreamPtr[ins.size()+1];
	for (i=0; i<ins.size(); i++) {
	    _ins[i] = ins[i]->getStream();
	}
	_ins[i] = 0;

	StreamPtr* _outs = new StreamPtr[outs.size()+1];
	for (i=0; i<outs.size(); i++) {
	    _outs[i] = outs[i]->getStream();
	}
	_outs[i] = 0;

	__SKIR_call(kernel, _ins, _outs);

	delete[] _ins;
	delete[] _outs;
    }

    static int work(void *s, StreamPtr ins[], StreamPtr outs[]) {
	assert(0 && "error: called Kernel::work!");
	return 0;
    }

    void wait() {
	__SKIR_wait(kernel);
    }

    // create kernel
    Kernel(void) {
	kernel = __SKIR_kernel((void*)(&D::work), this);
    }

    ~Kernel(void) {
	kernel = 0;
    }
};
 
}
