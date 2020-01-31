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
// layout(line_strip, max_vertices = 2) out;
layout(triangle_strip, max_vertices = 90) out;

layout(location = 0) out vec3 v_color;

const vec3 basis[3] = { vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f) };
const vec2 offsets[] =
{
	vec2(1.f,0.f),
	vec2(0.5f,0.866025f),
	vec2(-0.5f,0.866025f),
	vec2(-1.f,0.f),
	vec2(-0.5f,-0.866025f),
	vec2(0.5f,-0.866025f)
};
const int NFACES = 6;
const float f_cyl_diameter = 0.01f;
const float f_cyl_length = 2.f;
const float f_arrow_diameter = 0.1f;
const float f_arrow_length = 0.3f;

void main()
{
	// Local basis
	vec3 t = basis[gl_PrimitiveIDIn];
	vec3 r = basis[(gl_PrimitiveIDIn+1)%3];
	vec3 s = basis[(gl_PrimitiveIDIn+2)%3];

	v_color = t.xyz;

	for(int ii=0; ii<NFACES; ++ii)
	{
		vec3 offset      = offsets[ii].x * r + offsets[ii].y * s;
		vec3 next_offset = offsets[(ii+1)%NFACES].x * r + offsets[(ii+1)%NFACES].y * s;

		vec4 base            = u_m4_mvp*vec4(f_cyl_diameter * offset, 1.f);
		vec4 top             = u_m4_mvp*vec4(f_cyl_length * t + f_cyl_diameter * offset, 1.f);
		vec4 next_base       = u_m4_mvp*vec4(f_cyl_diameter * next_offset, 1.f);
		vec4 next_top        = u_m4_mvp*vec4(f_cyl_length * t + f_cyl_diameter * next_offset, 1.f);
    	vec4 arrow_base      = u_m4_mvp*vec4(f_cyl_length * t + f_arrow_diameter * offset, 1.f);
    	vec4 next_arrow_base = u_m4_mvp*vec4(f_cyl_length * t + f_arrow_diameter * next_offset, 1.f);
    	vec4 head            = u_m4_mvp*vec4((f_cyl_length + f_arrow_length) * t, 1.f);

		// Cylinder
		gl_Position = base;
		EmitVertex();
		gl_Position = next_base;
		EmitVertex();
		gl_Position = top;
		EmitVertex();
    	EndPrimitive();

		gl_Position = next_base;
		EmitVertex();
		gl_Position = next_top;
		EmitVertex();
		gl_Position = top;
		EmitVertex();
    	EndPrimitive();

    	// Cone
		gl_Position = arrow_base;
		EmitVertex();
		gl_Position = next_arrow_base;
		EmitVertex();
		gl_Position = head;
		EmitVertex();
    	EndPrimitive();

    	// Cone, behind
		gl_Position = next_top;
		EmitVertex();
		gl_Position = next_arrow_base;
		EmitVertex();
		gl_Position = top;
		EmitVertex();
    	EndPrimitive();

		gl_Position = top;
		EmitVertex();
		gl_Position = next_arrow_base;
		EmitVertex();
		gl_Position = arrow_base;
		EmitVertex();
    	EndPrimitive();
	}
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