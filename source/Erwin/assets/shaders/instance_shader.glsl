#type vertex
#version 460 core

struct InstanceData
{
    vec4 uvs;
    vec4 tint;
    vec4 offset;
    vec2 scale;
    vec2 padding;
};

layout(std430, binding = 0) buffer instance_data
{
    InstanceData inst[];
};

layout(std140, binding = 0) uniform matrices
{
    mat4 u_view_projection;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec4 v_tint;

void main()
{
    vec3 offset = inst[gl_InstanceID].offset.xyz;
    vec3 scale  = vec3(inst[gl_InstanceID].scale, 1.f);
    vec4 uvs    = inst[gl_InstanceID].uvs;
    v_tint      = inst[gl_InstanceID].tint;

    gl_Position = u_view_projection*vec4(in_position*scale + offset, 1.f);
    v_uv.x = (in_uv.x < 0.5f) ? uvs.x : uvs.z;
    v_uv.y = (in_uv.y < 0.5f) ? uvs.y : uvs.w;
}

#type fragment
#version 460 core
#include "engine/common.glsl"

SAMPLER_2D_(0);  // Atlas (diffuse)

layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec4 v_tint;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(SAMPLER_2D_0, v_uv) * v_tint;
    // out_color = vec4(v_uv, 0.f, 1.f);
    // out_color = vec4(1.f, 0.f, 0.f, 1.f);
}
