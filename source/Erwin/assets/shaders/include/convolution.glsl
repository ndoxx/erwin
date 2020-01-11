#define KERNEL_MAX_WEIGHTS 5
vec4 convolve_kernel_separable_rgba(float weight[KERNEL_MAX_WEIGHTS], int half_size,
                                    sampler2D samp, vec2 v2_uv, vec2 v2_offset)
{
    // Fragment's central contribution
    vec4 result = texture(samp, v2_uv) * weight[0];

    // Add (symmetric) side contributions
    for(int ii=1; ii<half_size; ++ii)
    {
        result += texture(samp,  ii*v2_offset + v2_uv) * weight[ii];
        result += texture(samp, -ii*v2_offset + v2_uv) * weight[ii];
    }

    return result;
}

#define KERNEL_MAX_PACKED_WEIGHTS 3
vec4 convolve_kernel_separable_rgba_packed(vec4 weight[KERNEL_MAX_PACKED_WEIGHTS], int half_size,
                                           sampler2D samp, vec2 v2_uv, vec2 v2_offset)
{
    // Fragment's central contribution
    vec4 result = texture(samp, v2_uv) * weight[0][0];

    // Add (symmetric) side contributions
    for(int ii=1; ii<half_size; ++ii)
    {
        result += texture(samp,  ii*v2_offset + v2_uv) * weight[ii/4][ii%4];
        result += texture(samp, -ii*v2_offset + v2_uv) * weight[ii/4][ii%4];
    }

    return result;
}