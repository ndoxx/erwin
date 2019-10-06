#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) out vec3 v_color;

void main()
{
	gl_Position = vec4(in_position, 1.f);
	v_color = in_color;
}

#type fragment
#version 460 core

layout(location = 2) in vec3 v_color;
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(v_color,1.0f);
}
