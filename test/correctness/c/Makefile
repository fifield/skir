
##
##
##
LEVEL = ../../..

SKIR_OPT_LVL=2
SKIR_DEFS=-DCORRECTNESS=1
SKIR_OUT=./output

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common
include ../../Makefile.SKIR

check-local:: fir_pipe-skir.log

# clean
clean::
	rm -f *.bc *.log *.run *.ll *.dot

# RUNS

%-skir.run: %-opt.bc
	echo "${SKIR_LLI} $< 10000" > $@

fir_pipe-skir.run: fir_pipe-opt.bc
	echo "${SKIR_LLI} $< 10000 16 1024" > $@

fir_tree-skir.run: fir_pipe-opt.bc
	echo "${SKIR_LLI} $< 10000 16 1024" > $@

loop_decimate_add-skir.run: loop_decimate_add-opt.bc
	echo "${SKIR_LLI} $< 10000 10" > $@

simple_hello-skir.run: simple_hello-opt.bc
	echo "${SKIR_LLI} $< 20" > $@

blackscholes-skir.run: blackscholes-opt.bc
	echo "${SKIR_LLI} $< 2000 blackscholes_input_4.txt" > $@

#blackscholes-skir.log: blackscholes-opt.run

# LOGS

%.log: %.run
	rm -f $@
	sh $< 2>&1 | tee $@
	$(PROJ_SRC_ROOT)/test/correctness/fdiff.py $@ \
		$(PROJ_SRC_ROOT)/test/correctness/c/output/$@
