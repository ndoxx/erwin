#type vertex
#version 460 core

struct InstanceData
{
    vec2 offset;
    vec2 scale;
    vec4 uvs;
};

layout(std430, binding = 0) buffer instance_data
{
    InstanceData inst[];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) out vec2 v_uv;

uniform mat4 u_view_projection;

void main()
{
    vec3 offset = vec3(inst[gl_InstanceID].offset, 0.f);
    vec3 scale  = vec3(inst[gl_InstanceID].scale, 1.f);
    vec4 uvs    = inst[gl_InstanceID].uvs;

    gl_Position = u_view_projection*vec4(in_position*scale + offset, 1.f);
    v_uv.x = (in_uv.x < 0.5f) ? uvs.x : uvs.z;
  	v_uv.y = (in_uv.y < 0.5f) ? uvs.y : uvs.w;
}

#type fragment
#version 460 core

layout(binding = 0) uniform sampler2D us_atlas;
layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(us_atlas, v_uv);
}
