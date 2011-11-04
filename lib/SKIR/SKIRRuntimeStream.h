#ifndef _SKIR_RUNTIME_STREAM_H_
#define _SKIR_RUNTIME_STREAM_H_

#include <SKIR/SKIRStream.h>

//#define SKIR_SHM_STREAMS 1

//#ifdef SKIR_SHM_STREAMS
#include <iostream>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>
//#endif

#include <llvm/Support/raw_ostream.h>

#include <assert.h>
#include <stdio.h>
#include <tbb/tbb.h>

typedef tbb::spin_mutex stream_lock_t;

namespace llvm {

// respresentation of streams in the runtime graph

struct SKIRRuntimeStream
{
    SKIRRuntimeStream(unsigned id) :
    si(0), id(id), type(-1),
	is_src(false), is_dst(false),
	elem_size(0), stride(0), //begin(0), end(0),
	pop_rate(-1), push_rate(-1), peek_rate(0),
        readtagchanged(false), writetagchanged(false), qsize(STREAM_BUFFER_SIZE)
    {}

    enum Type {
	NATIVE = 0,  // intra process
	SHARED = 1,  // inter process
	SOCKET = 2,  // inter machine
	ERR   = -1
    };
    
    // pointer to stream implementation
    skir_stream_t *si;

    unsigned id;
    int type;

    bool is_src;
    bool is_dst;

    // for array streams
    unsigned int elem_size;
    unsigned int stride;
    //void *begin;
    //void *end;

    // for D4R
    bool readtagchanged;
    bool writetagchanged;
    int qsize;

    int getPopRate() { return pop_rate; }
    int getPushRate() { return push_rate; }
    int getPeekRate() { return peek_rate; }    

    void setPopRate(int r) { pop_rate = r; }
    void setPushRate(int r) { push_rate = r; }
    void setPeekRate(int r) { peek_rate = r; }    

private:
    // these are private so that they will not
    // be confused with fields of the same name 
    // in skir_stream_t
    int pop_rate;
    int push_rate;
    int peek_rate;

    SKIRRuntimeStream() {}
    SKIRRuntimeStream(const SKIRRuntimeStream &s) {}
};

inline void
free_skir_stream_t(skir_stream_t *s)
{
    SKIRRuntimeStream *rs = (SKIRRuntimeStream *)s->rs;

    bool src_dead = (s->src == (void*)1);
    bool dst_dead = (s->dst == (void*)1);

    assert( (src_dead || dst_dead) && "free_skir_stream_t called on live stream");

    if (!(src_dead && dst_dead))
	return;

    s->rs = 0;
    rs->si = 0;

    if (rs->type == SKIRRuntimeStream::NATIVE) {
	free(s);
    }
    else if (rs->type == SKIRRuntimeStream::SHARED) {
	size_t header_size = sizeof(skir_stream_t)*(NUM_STREAM_HEADERS+1);
	size_t buffer_size = STREAM_BUFFER_SIZE;
	size_t alloc_size = header_size + buffer_size;
	munmap(s, alloc_size);
	std::stringstream file_name;
	file_name << "/skir_stream." << rs->id;
	shm_unlink(file_name.str().c_str());
    }

}

inline skir_stream_t *
new_skir_stream_t(SKIRRuntimeStream *rs)
{
    assert(rs);
    if (rs->type == SKIRRuntimeStream::ERR)
	return 0;

    size_t header_size = sizeof(skir_stream_t)*(NUM_STREAM_HEADERS+1);
    size_t buffer_size = STREAM_BUFFER_SIZE;
    size_t alloc_size = header_size + buffer_size;

    skir_stream_t *s;
    if (rs->type == SKIRRuntimeStream::NATIVE) {
	s = (skir_stream_t*)malloc(alloc_size);
	s->id = 0;
    }
    else if (rs->type == SKIRRuntimeStream::SHARED) {
	int fd;
	std::stringstream file_name;
	file_name << "/skir_stream." << rs->id;
	fd = shm_open( file_name.str().c_str(), O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
	assert(fd >= 0);
	assert( ftruncate(fd, alloc_size) == 0 );

	s = (skir_stream_t*)mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	assert(s != MAP_FAILED);

	// this is for resetting the stream header?
	//if ((s->src == (void *)1) && (s->dst == (void*)1))
	//    s->id = 0;
    }
    assert(s);

    char *buf = ((char*)s) + header_size;

    for (int i=0; i<NUM_STREAM_HEADERS+1; i++, s++) {
	s->rs = rs;
	s->outp = buf;
	s->inp = buf;

	if (s->id) continue;
	s->id = i+1;

	s->elem_size = rs->elem_size;
	s->stride = 0;

	s->pop_rate = rs->getPopRate();
	s->push_rate = rs->getPushRate();
	s->peek_rate = rs->getPeekRate();

	s->src = s->dst = (void *)(-1);

	s->head = 0;
	s->outp = buf;
	s->num_push = 0;
	s->next_tail = 0;

	s->tail = 0;
	s->inp = buf;
	s->next_head = 0;
    }
    --s;
    return s;
}	

inline skir_stream_t *
copy_skir_stream_t(skir_stream_t *s)
{
    return new_skir_stream_t((SKIRRuntimeStream*)s->rs);
}    

}
#endif
