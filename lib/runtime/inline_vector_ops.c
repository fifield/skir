
//
// split/join vectorization
//

typedef unsigned int uint4 __attribute__((ext_vector_type(4)));

void __split_2_1_vec4x4 (uint4 *in, uint4 *out0, uint4 *out1)
{
    out0[0] = __builtin_shufflevector(in[0], in[1], 0, 2, 4, 6);
    out1[0] = __builtin_shufflevector(in[0], in[1], 1, 3, 5, 7);
}

void __split_4_1_vec4x4 (uint4 *in, uint4 *out0, uint4 *out1, uint4 *out2, uint4 *out3)
{
    uint4 tmp0 = __builtin_shufflevector(in[0], in[1], 0, 4, 1, 5);
    uint4 tmp1 = __builtin_shufflevector(in[2], in[3], 0, 4, 1, 5);

    uint4 tmp2 = __builtin_shufflevector(in[0], in[1], 2, 6, 3, 7);
    uint4 tmp3 = __builtin_shufflevector(in[2], in[3], 2, 6, 3, 7);

    out0[0] =  __builtin_shufflevector(tmp0, tmp1, 0, 1, 4, 5);
    out1[0] =  __builtin_shufflevector(tmp0, tmp1, 2, 3, 6, 7);

    out2[0] =  __builtin_shufflevector(tmp2, tmp3, 0, 1, 4, 5);
    out3[0] =  __builtin_shufflevector(tmp2, tmp3, 2, 3, 6, 7);
}

void __split_2_4_vec4x4 (uint4 *in, uint4 *out0, uint4 *out1)
{
    out0[0] = in[0];
    out1[0] = in[1];
}

void __split_4_2_vec4x4 (uint4 *in, uint4 *out0, uint4 *out1, uint4 *out2, uint4 *out3)
{
    out0[0] =  __builtin_shufflevector(in[0], in[1], 0, 1, 0, 1);
    out1[0] =  __builtin_shufflevector(in[0], in[1], 2, 3, 2, 3);
    out2[0] =  __builtin_shufflevector(in[0], in[1], 4, 5, 4, 5);
    out3[0] =  __builtin_shufflevector(in[0], in[1], 6, 7, 6, 7);
} 

void __join_2_1_vec4x4 (uint4 *in0, uint4 *in1, uint4 *out)
{
    out[0] = __builtin_shufflevector(in0[0], in1[0], 0, 4, 1, 5);
    out[1] = __builtin_shufflevector(in0[0], in1[0], 2, 6, 3, 7);
}

void __join_4_1_vec4x4 (uint4 *in0, uint4 *in1, uint4 *in2, uint4 *in3, uint4 *out)
{
    uint4 tmp0 = __builtin_shufflevector(in0[0], in1[0], 0, 4, 1, 5);
    uint4 tmp1 = __builtin_shufflevector(in2[0], in3[0], 0, 4, 1, 5);

    uint4 tmp2 = __builtin_shufflevector(in0[0], in1[0], 2, 6, 3, 7);
    uint4 tmp3 = __builtin_shufflevector(in2[0], in3[0], 2, 6, 3, 7);
    
    out[0] = __builtin_shufflevector(tmp0, tmp1, 0, 1, 4, 5);
    out[1] = __builtin_shufflevector(tmp0, tmp1, 2, 3, 6, 7);

    out[2] =  __builtin_shufflevector(tmp2, tmp3, 0, 1, 4, 5);
    out[3] =  __builtin_shufflevector(tmp2, tmp3, 2, 3, 6, 7);

}

void __join_2_4_vec4x4 (uint4 *in0, uint4 *in1, uint4 *out)
{
    out[0] = in0[0];
    out[1] = in1[1];
}

void __join_4_2_vec4x4 (uint4 *in0, uint4 *in1, uint4 *in2, uint4 *in3, uint4 *out)
{
    out[0] = __builtin_shufflevector(in0[0], in1[0], 0, 1, 4, 5);
    out[1] = __builtin_shufflevector(in2[0], in3[0], 0, 1, 4, 5);
    out[2] = __builtin_shufflevector(in0[0], in1[0], 2, 3, 6, 7);
    out[3] = __builtin_shufflevector(in2[0], in3[0], 2, 3, 6, 7);
}

#ifdef INLINE_VECTOR_OPS_TEST

int main(int argc, char *argc[])
{

}

#endif
