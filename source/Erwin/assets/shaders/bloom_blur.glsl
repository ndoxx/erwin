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

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;
SAMPLER_2D_(0);

layout(std140, binding = 0) uniform blur_data
{
	vec2 u_v2_offset;
};

void main()
{
	vec4 col = vec4(0.f);
	col += 5.f * texture(SAMPLER_2D_0, v_uv - u_v2_offset);
	col += 6.f * texture(SAMPLER_2D_0, v_uv);
	col += 5.f * texture(SAMPLER_2D_0, v_uv + u_v2_offset);
	out_color = col/16.f;
}