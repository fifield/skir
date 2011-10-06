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

PROTOBUF_CFLAGS=-pthread -I$(LibDir) -I/usr/local/include
PROTOBUF_LIBS=-pthread -lprotobuf -lz  -levents_pb  -L$(LibDir) -L/usr/local/lib

CXXFLAGS+=-I$(LibDir) -I$(PROJ_SRC_DIR)/$(LEVEL)/deps/tbb/include
CXXFLAGS+=$(OPENCL_CFLAGS) $(PROTOBUF_CFLAGS) -fexceptions

#LDFLAGS+=-ltbb_debug -ltbbmalloc_debug
#LDFLAGS+=-L$(PROJ_OBJ_ROOT)/$(BuildMode)/tbb_build/debug
LDFLAGS+=-ltbb -ltbbmalloc
LDFLAGS+=-L$(PROJ_OBJ_ROOT)/$(BuildMode)/tbb_build/release

LDFLAGS+=$(OPENCL_LDFLAGS) $(PROTOBUF_LIBS)
LDFLAGS+=-lboost_system-mt -lboost_thread-mt

