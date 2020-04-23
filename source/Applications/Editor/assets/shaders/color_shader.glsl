#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) out vec3 v_color;
layout(location = 3) out vec2 v_uv;

layout(std140, binding = 0) uniform matrices
{
    mat4 u_mvp;
};

void main()
{
	gl_Position = u_mvp*vec4(in_position, 1.f);
	v_color = vec3(in_uv, 0.f);
	v_uv = in_uv;
}

#type fragment
#version 460 core

layout(location = 2) in vec3 v_color;
layout(location = 3) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(v_color, 1.f);
	// out_color = vec4(1.f,0.f,0.f,1.f);
}
