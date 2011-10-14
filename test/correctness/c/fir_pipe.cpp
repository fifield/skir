#include <assert.h>
#include <iostream>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "skir_intrinsics.h"
#include "filters.h"
#include "simple_kernels.h"


void
balanced_fir_test(int nfir, int ntaps, int nsamples)
{
    assert(nfir <= 1024);
    assert(ntaps == 1024);

    //stream_graph_t *graph = new( tbb::task::allocate_root() ) stream_graph_t();

    int int0 = nsamples;

    skir_kernel_ptr_t src;
    skir_kernel_ptr_t fir[1024];
    skir_kernel_ptr_t sink;

    skir_stream_ptr_t ins[2] = {0, 0};
    skir_stream_ptr_t outs[2] = {0, 0};

    //src = graph->add_kernel(simple_0_1_work, &int0);
    src = __SKIR_kernel((void*)simple_0_1_work, &int0);

    //graph->add_stream(src, fir[0]);
    outs[0] = __SKIR_stream(sizeof(int));
    __SKIR_call(src, ins, outs);
    ins[0] = outs[0];

    int nkern = 0;
    for (int i=0; i<nfir; nkern++, i++) {
	fir[nkern] = __SKIR_kernel((void*)fir1024_1_1_work,
				   new fir_state_t(ntaps));
	outs[0] = __SKIR_stream(sizeof(int));
	__SKIR_call(fir[nkern], ins, outs);
	ins[0] = outs[0];
    }    

#ifdef CORRECTNESS
    sink = __SKIR_kernel((void*)int_printer_work, new int_printer_t(""));
#else
    simple_1_0_t t;
    t.div = 100000;
    sink = __SKIR_kernel((void*)simple_1_0_work, &t);
#endif
    outs[0] = 0;//__SKIR_stream(sizeof(int));
    __SKIR_call(sink, ins, outs);

    __SKIR_wait(sink);
}

int main(int argc, char *argv[])
{
    if (argc !=4) {
	printf("usage:\n%s\t<nsamples> <nfir> <ntaps>\n", argv[0]);
	return EXIT_FAILURE;
    }
	       
    int nsamples = atoi(argv[1]);
    int nfir     = atoi(argv[2]);
    int ntaps   = atoi(argv[3]);

    balanced_fir_test(nfir, ntaps, nsamples);

    return EXIT_SUCCESS;
}
