#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

void main()
{
    gl_Position = vec4(a_position, 1.f);
}


#type geometry
#version 460 core

layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 4) out;

void main() 
{     
    const vec2 vert_data[4] = { vec2(-1.f, 1.f), vec2(-1.f, -1.f), vec2(1.f, 1.f), vec2(1.f, -1.f) };

    for(int ii=0; ii<4; ++ii)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = vec4(vert_data[ii],0.f,1.f);
        EmitVertex();
    }

    EndPrimitive();
}


#type fragment
#version 460 core
#include "engine/common.glsl"
#include "engine/sphere_map.glsl"

layout(location = 0) out vec4 out_color;

SAMPLER_2D_(0); // Equirectangular HDR texture

layout(std140, binding = 0) uniform parameters
{
	vec2 u_v2_viewport_size;
};

void main()
{
	vec2 uv = (gl_FragCoord.xy/u_v2_viewport_size)*2.f-1.f;
	vec3 pos_w = to_world_vector(uv, gl_Layer);
    vec2 s_uv = sample_spherical_map(pos_w);
    vec3 color = texture(SAMPLER_2D_0, s_uv).rgb;

    out_color = vec4(color, 1.f);
}