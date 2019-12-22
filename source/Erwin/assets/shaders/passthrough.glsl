#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) out vec2 v_uv;

void main()
{
	gl_Position = vec4(in_position, 1.f);
	v_uv = in_uv;
}

#type fragment
#version 460 core

#include "include/common.glsl"

layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

SAMPLER_2D_(0);

void main()
{
	out_color = texture(SAMPLER_2D_0, v_uv);
}