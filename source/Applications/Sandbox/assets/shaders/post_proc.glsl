#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 v_uv;

void main()
{
	gl_Position = vec4(in_position, 1.f);
	v_uv = in_uv;
}

#type fragment
#version 460 core

in vec2 v_uv;
layout(location = 0) out vec4 out_color;

uniform sampler2D us_input;

void main()
{
	vec4 in_color = texture(us_input, v_uv);

    out_color = in_color;
}