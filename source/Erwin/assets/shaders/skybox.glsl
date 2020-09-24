#type vertex
#version 460 core
#include "engine/frame_ubo.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 0) out vec3 v_uv3;

void main()
{
    v_uv3 = a_position;
    vec4 pos = u_m4_aavp * vec4(a_position, 1.0);
    // z component always 1 after perspective divide
    // NOTE(ndx): requires "less equal" depth func
    gl_Position = pos.xyww;
}


#type fragment
#version 460 core
#include "engine/common.glsl"

SAMPLER_CUBE_(0); // The cubemap to display as a skybox

layout (location = 0) in vec3 v_uv3;
layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_glow;

void main()
{
    vec3 env_color = texture(SAMPLER_CUBE_0, v_uv3).rgb;

    // Tonemap and gamma correction
    // env_color = env_color / (env_color + vec3(1.f));
    // env_color = pow(env_color, vec3(1.f/2.2f));

    out_color = vec4(env_color, 1.f);

    out_glow = vec4(0.f);
}