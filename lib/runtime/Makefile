
LEVEL=../..

include $(LEVEL)/Makefile.common

#
# protobuf
#

PROTOC=/usr/local/bin/protoc
PROTOC_INC=/usr/local/include

skir-protobuf: $(LibDir)/libevents_pb.so

$(LibDir)/events.pb.cc:
	$(Echo) Building Protocol Buffer Library for $(BuildMode) Build
	$(Verb) $(MKDIR) $(LibDir)
	$(Verb) $(PROTOC) -o$(LibDir)/events.desc --python_out=$(LibDir) --cpp_out=$(LibDir) \
		--proto_path=$(PROJ_SRC_ROOT)/lib/runtime \
		$(PROJ_SRC_ROOT)/lib/runtime/events.proto

$(LibDir)/events.pb.o: $(LibDir)/events.pb.cc
	$(Verb) $(CXX) -I$(PROTOC_INC) -O2 -fPIC -c $(LibDir)/events.pb.cc \
		-o $(LibDir)/events.pb.o

$(LibDir)/libevents_pb.so: $(LibDir)/events.pb.o
	$(Verb) $(CC) -shared -W1,-soname,libevents_pb.so \
		-o $(LibDir)/libevents_pb.so $(LibDir)/events.pb.o

#
# runtime libs
#

LIBS_CFLAGS=-O0 -fPIC -D_COMPILE_ME_ -I $(PROJ_SRC_ROOT)/include -c

skir-lib: skir-protobuf \
	$(LibDir)/inline_stream_ops.h \
	$(LibDir)/inline_stream_ops.cpp \
	$(LibDir)/inline_stream_ops.bc \
	$(LibDir)/inline_stream_ops.ll \
	$(LibDir)/skir_opencl_mod.bc \
	$(LibDir)/skir_opencl_mod.ll

$(LibDir)/libinline_stream_ops.so: $(LibDir)/inline_stream_ops.o
	$(Verb) $(CC) -shared -W1,-soname,libinline_stream_ops.so -o $@ $<

$(LibDir)/inline_stream_ops.o: $(PROJ_SRC_ROOT)/lib/runtime/inline_stream_ops.cpp
	$(Verb) $(CXX) ${LIBS_CFLAGS} -o $@ $<

$(LibDir)/inline_stream_ops.c.bc: $(PROJ_SRC_ROOT)/lib/runtime/inline_stream_ops.cpp
	$(Echo) Compiling SKIR Runtime Bitcode Library for $(BuildMode) Build
	$(Verb) $(LLVMCXX) ${LIBS_CFLAGS} -Wno-div-by-zero -emit-llvm -o $@ $<

$(LibDir)/inline_stream_ops.ll: $(LibDir)/inline_stream_ops.c.bc
	$(Verb) opt -skir-c-to-intr < $< | opt  -O2 | llvm-dis > $@
#	$(Verb) opt -skir-c-to-intr < $< | opt -disable-inlining -std-compile-opts | llvm-dis > $@

$(LibDir)/inline_stream_ops.bc: $(LibDir)/inline_stream_ops.ll
	$(Verb) llvm-as < $< > $@

$(LibDir)/inline_stream_ops.h: $(PROJ_SRC_ROOT)/lib/runtime/inline_stream_ops.h
	$(Verb) cp $< $@

$(LibDir)/inline_stream_ops.cpp: $(PROJ_SRC_ROOT)/lib/runtime/inline_stream_ops.cpp
	$(Verb) cp $< $@

%.ll: %.bc
	$(Verb) llvm-dis < $< > $@

$(LibDir)/skir_opencl_mod.bc: $(PROJ_SRC_ROOT)/lib/runtime/skir_opencl_mod.ll
	$(Echo) Compiling OpenCL Runtime Bitcode Library for $(BuildMode) Build
	$(Verb) llvm-as < $< > $@

all:: skir-lib
