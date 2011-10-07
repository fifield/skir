#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "skir_intrinsics.h"

void *
hello_init(void *a)
{
    int *i = (int *)malloc(sizeof(int));
    memcpy(i, a, sizeof(int));
    return i;
}

int
hello_work(void *s, skir_stream_ptr_t ins[], skir_stream_ptr_t outs[])
{

    int *i = (int *)s;
    printf("hello, world %d\n", *i);
    return 1;
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
	printf("usage: %s <num hello>\n", argv[0]);
	exit(0);
    }

    int num_hello = atoi(argv[1]);

    printf("num_hello: %d\n", num_hello);
    
    skir_stream_ptr_t ins[2] = {0, 0};
    skir_stream_ptr_t outs[2] = {0, 0};

    int i = 0;
    skir_kernel_ptr_t h = __SKIR_kernel((void*)hello_init, (void*)hello_work, &i);
    //outs[0] = __SKIR_stream(sizeof(int));
    __SKIR_call(h, ins, outs);
    //ins[0] = outs[0];

    for (i=1; i<num_hello; i++) {
	h = __SKIR_kernel((void*)hello_init, (void*)hello_work, &i);
	//outs[0] = __SKIR_stream(sizeof(int));
	__SKIR_call(h, ins, outs);
	//ins[0] = outs[0];
    }

    //outs[0] = 0;
    h = __SKIR_kernel((void*)hello_init, (void*)hello_work, &i);
    __SKIR_call(h, ins, outs);
    
    __SKIR_wait(h);
    return 0;
}

