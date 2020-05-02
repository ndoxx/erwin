#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv; // Texture coordinates


layout(std140, binding = 0) uniform parameters
{
	int u_flags;
};

void main()
{
    gl_Position = vec4(a_position, 1.f);
    v_uv = a_uv;
}



#type fragment
#version 460 core
#include "engine/common.glsl"

SAMPLER_2D_(0); // Albedo
SAMPLER_2D_(1); // Normal
SAMPLER_2D_(2); // Depth
SAMPLER_2D_(3); // Metal
SAMPLER_2D_(4); // AO
SAMPLER_2D_(5); // Roughness
SAMPLER_2D_(6); // Emissivity

#define TMF_ALBEDO     1<<0
#define TMF_NORMAL     1<<1
#define TMF_DEPTH      1<<2
#define TMF_METAL      1<<3
#define TMF_AO         1<<4
#define TMF_ROUGHNESS  1<<5
#define TMF_EMISSIVITY 1<<6

layout(location = 0) in vec2 v_uv; // Texture coordinates

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal_depth;
layout(location = 2) out vec4 out_mare;

layout(std140, binding = 0) uniform parameters
{
    int u_flags;
};

void main()
{
    if(bool(u_flags & TMF_ALBEDO))
        out_albedo = texture(SAMPLER_2D_0, v_uv);
    else
        out_albedo = vec4(1.f);

    if(bool(u_flags & TMF_NORMAL))
        out_normal_depth.xyz = texture(SAMPLER_2D_1, v_uv).xyz;
    else
        out_normal_depth.xyz = vec3(0.f);

    if(bool(u_flags & TMF_DEPTH))
        out_normal_depth.w = texture(SAMPLER_2D_2, v_uv).x;
    else
        out_normal_depth.w = 0.f;

    if(bool(u_flags & TMF_METAL))
        out_mare.x = texture(SAMPLER_2D_3, v_uv).x;
    else
        out_mare.x = 0.f;

    if(bool(u_flags & TMF_AO))
        out_mare.y = texture(SAMPLER_2D_4, v_uv).x;
    else
        out_mare.y = 1.f;

    if(bool(u_flags & TMF_ROUGHNESS))
        out_mare.z = texture(SAMPLER_2D_5, v_uv).x;
    else
        out_mare.z = 0.5f;

    if(bool(u_flags & TMF_EMISSIVITY))
        out_mare.w = texture(SAMPLER_2D_6, v_uv).x;
    else
        out_mare.w = 0.f;
}