##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=LLVMSKIR

NO_PEDANTIC=1

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

OPENCL_CFLAGS=-I$(OPENCL_PATH)/include 
OPENCL_LDFLAGS=-L$(OPENCL_PATH)/lib/x86_64 -lOpenCL

PROTOBUF_CFLAGS=-I/usr/local/include
PROTOBUF_LIBS=-pthread -lprotobuf -lz -L/usr/local/lib

CXXFLAGS+=-I$(PROJ_SRC_DIR)/$(LEVEL)/deps/tbb/include -DUSE_OPENCL $(OPENCL_CFLAGS)
CXXFLAGS+=-DUSE_TBB=$(USE_TBB)  $(PROTOBUF_CFLAGS) -I$(LibDir) -Wno-div-by-zero

LDFLAGS+=$(OPENCL_LDFLAGS) $(PROTOBUF_LIBS)
