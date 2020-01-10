#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 0) out vec2 v_uv;

void main()
{
	gl_Position = vec4(a_position, 1.f);
	v_uv = a_uv;
}

#type fragment
#version 460 core
#include "include/common.glsl"
#include "include/convolution.glsl"

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;
SAMPLER_2D_(0);

layout(std140, binding = 0) uniform blur_data
{
	vec2 u_v2_offset;
    int u_i_half_size;
    int padding;
	float u_f_weight[KERNEL_MAX_WEIGHTS];
};

// TMP: passing as uniform fails atm
// const float weights[] = { 0.387745f, 0.244769f, 0.0613584f, 0,0,0,0,0,0,0,0,0,0,0,0 };
const float weights[] = { 0.382932f, 0.241730f, 0.0605967f, 0.00597757f, 0.000229347f, 0,0,0,0,0,0,0,0,0,0 };

void main()
{
	out_color = convolve_kernel_separable_rgba(weights, 5, SAMPLER_2D_0, v_uv, u_v2_offset);
	// out_color = convolve_kernel_separable_rgba(u_f_weight, u_i_half_size, SAMPLER_2D_0, v_uv, u_v2_offset);
}