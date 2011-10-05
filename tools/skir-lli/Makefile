##===- skir-lli/tools/skir-lli/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of the tool.
#
TOOLNAME=skir-lli

LINK_COMPONENTS := bitreader scalaropts ipo jit instrumentation native asmparser

NO_PEDANTIC=1

USEDLIBS=LLVMSKIR.a

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

OPENCL_CFLAGS=-I$(OPENCL_PATH)/include 
OPENCL_LDFLAGS=-L$(OPENCL_PATH)/lib/x86_64 -lOpenCL

#OPENCL_CFLAGS=
#OPENCL_LDFLAGS=

PROTOBUF_CFLAGS=-pthread -I$(LibDir) -I/usr/local/include
PROTOBUF_LIBS=-pthread -lprotobuf -lz  -levents_pb  -L$(LibDir) -L/usr/local/lib

CXXFLAGS+=-I$(LibDir) -I$(TBB_PATH)/include $(OPENCL_CFLAGS) $(PROTOBUF_CFLAGS) 
CXXFLAGS+=-I/usr/include/boost -fexceptions

#LDFLAGS+=-ltbb_debug -ltbbmalloc_debug -L$(TBB_PATH)/build/debug 
LDFLAGS+=-ltbb -ltbbmalloc -L$(TBB_PATH)/build/release

LDFLAGS+=$(OPENCL_LDFLAGS) $(PROTOBUF_LIBS)
LDFLAGS+=-lboost_system-mt -lboost_thread-mt

