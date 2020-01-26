#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;

layout(std140, binding = 0) uniform line_data
{
	mat4 u_m4_mvp;
	vec4 u_v4_color;
};

void main()
{
	gl_Position = u_m4_mvp*vec4(a_position, 1.f);
}

#type fragment
#version 460 core

layout(std140, binding = 0) uniform line_data
{
	mat4 u_m4_mvp;
	vec4 u_v4_color;
};

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_glow;

void main()
{
    out_color = u_v4_color;
    out_glow = vec4(0.f,0.f,0.f,0.f);
}