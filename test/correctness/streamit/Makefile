
##
##
##
LEVEL = ../../..

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common

include ../../Makefile.SKIR

check-local:: test

test: test_simple_streamit #test_streamit

clean::
	rm -f *.bc *.log *.run

#
# streamit tests
#

test_streamit:
	make -C beamformer test
	make -C bitonic-sort test
#	make -C channelvocoder test
	make -C dct test
	make -C des test
	make -C fft test
	make -C filterbank test
	make -C fm test
	make -C mpeg2-subset test
#	make -C serpent_full test
#	make -C tde_pp test
#	make -C vocoder test

clean_streamit:
	make -C beamformer clean
	make -C bitonic-sort clean
	make -C channelvocoder clean
	make -C dct clean
	make -C des clean
	make -C fft clean
	make -C filterbank clean
	make -C fm clean
	make -C mpeg2-subset clean
	make -C serpent_full clean
	make -C tde_pp clean
	make -C vocoder clean

#
# simple streamit tests
#

test_simple_streamit: test_array-skir.log test_pipe-skir.log test_anon-skir.log test_sj-skir.log

%.bc: %.str
	${SKIR_STREAMIT} < $< | llvm-as > $@

test_array-skir.run: test_array-skir.bc
	echo "${SKIR_LLI} $< | head -n 24" > $@

test_sj-skir.run: test_sj-skir.bc
	echo "${SKIR_LLI} $< | head -n 15" > $@

test_pipe-skir.run: test_pipe-skir.bc
	echo "${SKIR_LLI} $< | head -n 10" > $@

test_anon-skir.run: test_anon-skir.bc
	echo "${SKIR_LLI} $< | head -n 10" > $@

%.log: %.run
	rm -f $@
	sh $< 2>&1 | tee $@
	$(PROJ_SRC_ROOT)/test/correctness/fdiff.py $@ \
		$(PROJ_SRC_ROOT)/test/correctness/streamit/output/$@


