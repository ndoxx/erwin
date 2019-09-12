#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec3 v_color;
out vec2 v_uv;

void main()
{
	gl_Position = vec4(in_position, 1.f);
	v_color = vec3(in_uv, 0.f);
	v_uv = in_uv;
}

#type fragment
#version 460 core

in vec3 v_color;
in vec2 v_uv;
layout(location = 0) out vec4 out_color;

uniform sampler2D us_albedo;

void main()
{
	out_color = texture(us_albedo, v_uv);
}
