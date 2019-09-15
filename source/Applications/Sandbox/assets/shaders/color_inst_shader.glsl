#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 v_uv;
out vec3 v_color;

struct InstanceData
{
    vec2 offset;
    vec2 scale;
    vec4 color;
};

layout(std430) buffer instance_data
{
    InstanceData inst[];
};

void main()
{
    vec3 offset = vec3(inst[gl_InstanceID].offset, 0.f);
    vec3 scale  = vec3(inst[gl_InstanceID].scale, 1.f);

    gl_Position = vec4(in_position*scale + offset, 1.f);
  	v_uv = in_uv;
    v_color = inst[gl_InstanceID].color.rgb;
}

#type fragment
#version 460 core

in vec2 v_uv;
in vec3 v_color;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(v_color,1.0f);
}
