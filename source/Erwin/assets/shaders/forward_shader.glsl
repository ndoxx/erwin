#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;

/*layout(std140, binding = 0) uniform pass_data
{
	mat4 u_view_projection;
};*/
layout(std140, binding = 0) uniform instance_data
{
	mat4 u_mvp;  // model-view-projection
	vec4 u_tint; // tint
};

void main()
{
	gl_Position = u_mvp*vec4(in_position, 1.f);
}

#type fragment
#version 460 core

layout(location = 0) out vec4 out_color;

layout(std140, binding = 0) uniform instance_data
{
	mat4 u_mvp;  // model-view-projection
	vec4 u_tint; // tint
};

void main()
{
	out_color = u_tint;
}