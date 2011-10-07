##===- skir/tools/streamit/Makefile ---------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL=../..

# Give the name of the tool.
#
TOOLNAME=streamit

BUILT_SOURCES= StreamItLex.cpp StreamItParserFE.cpp

LINK_COMPONENTS = bitreader core jit native

USEDLIBS=LLVMSKIR.a

SOURCES=FEIRToSKIR.cpp main.cpp SymbolTable.cpp SymbolTableVisitor.cpp

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

#CXXFLAGS=-I./ -I./nodes -I/usr/include -fexceptions
CXXFLAGS=-fexceptions -I$(LibDir)
LIBS+=-lantlr

$(LibDir)/streamit.bc: $(PROJ_SRC_ROOT)/tools/streamit/streamit.cpp
	$(LLVMCXX) -O3 -fPIC -fno-exceptions -I$(LibDir) -I$(PROJ_SRC_ROOT)/include -emit-llvm -c -o $@ $<

StreamItLex.cpp: StreamItLex.g
	java -classpath /usr/share/java:/usr/share/java/antlr.jar antlr.Tool $<

StreamItParserFE.cpp: StreamItParserFE.g StreamItLex.cpp
	java -classpath /usr/share/java:/usr/share/java/antlr.jar antlr.Tool $<

clean::
	rm -f StreamItLex.cpp StreamItLex.hpp 
	rm -f StreamItLexTokenTypes.hpp StreamItLexTokenTypes.txt
	rm -f StreamItParserFE.cpp StreamItParserFE.hpp 
	rm -f StreamItParserFETokenTypes.hpp StreamItParserFETokenTypes.txt

all:: $(LibDir)/streamit.bc