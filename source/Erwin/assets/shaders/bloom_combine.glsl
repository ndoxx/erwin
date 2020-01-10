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
SAMPLER_2D_(1);
SAMPLER_2D_(2);

void main()
{
	vec4 col = 1.f*texture(SAMPLER_2D_0, v_uv)
			 + 2.f*texture(SAMPLER_2D_1, v_uv)
			 + 3.f*texture(SAMPLER_2D_2, v_uv);
	out_color = col/6.f;
}