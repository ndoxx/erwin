#type vertex
#version 460 core
#include "engine/transform_ubo.glsl"

layout(location = 0) in vec3 a_position;

void main()
{
	gl_Position = vec4(a_position, 1.f);
}



#type geometry
#version 460 core
#include "engine/transform_ubo.glsl"

layout(lines) in;
layout(line_strip, max_vertices = 2) out;
// layout(triangle_list, max_vertices = 6) out;

layout(location = 0) out vec3 v_color;

void main()
{
	v_color = gl_in[1].gl_Position.xyz;

    for(int ii=0; ii<gl_in.length(); ii++)
    {
    	gl_Position = u_m4_mvp*gl_in[ii].gl_Position;
   		EmitVertex();
    }
    EndPrimitive();
}



#type fragment
#version 460 core
#include "engine/transform_ubo.glsl"

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_glow;

void main()
{
    out_color = vec4(v_color,1.f);
    out_glow = vec4(0.f,0.f,0.f,0.f);
}