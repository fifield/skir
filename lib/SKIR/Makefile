##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=LLVMSKIR
#DONT_BUILD_RELINKED=1
#BUILD_ARCHIVE=1
NO_PEDANTIC=1

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

OPENCL_CFLAGS=-I$(OPENCL_PATH)/include 
OPENCL_LDFLAGS=-L$(OPENCL_PATH)/lib/x86_64 -lOpenCL

PROTOBUF_CFLAGS=-pthread -I/usr/local/include
PROTOBUF_LIBS=-pthread -lprotobuf -lz -L/usr/local/lib

CXXFLAGS+=-I$(TBB_PATH)/include -DUSE_OPENCL -DUSE_TBB=$(USE_TBB) $(OPENCL_CFLAGS) $(PROTOBUF_CFLAGS) -I$(LibDir) -Wno-div-by-zero

#LDFLAGS+=-ltbb_debug -ltbbmalloc_debug -L$(LEVEL)/../tbb/build/debug
LDFLAGS+=-ltbb -ltbbmalloc -L$(LEVEL)/../tbb/build/release
LDFLAGS+=$(OPENCL_LDFLAGS) $(PROTOBUF_LIBS)
