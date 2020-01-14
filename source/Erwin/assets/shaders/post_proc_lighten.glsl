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
#include "engine/common.glsl"

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;
SAMPLER_2D_(0);

void main()
{
	out_color = texture(SAMPLER_2D_0, v_uv);
}