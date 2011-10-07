
CORRECTNESS=1

all: exe test

include ./Makefile_c

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
	sh $< 2>&1 | head -n 10000 | tee $@ 
	../fdiff.py $@ output/$@
