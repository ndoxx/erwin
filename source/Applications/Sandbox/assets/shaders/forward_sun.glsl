#type vertex
#version 460 core
#include "include/forward_ubos.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv; // Texture coordinates

layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_sun_color;
};

void main()
{
    // z component always 1 after perspective divide
    // gl_Position = pos.xyww;

    // * Simple billboarding routine: draw a fixed size screen facing quad at the sun's position
    // Get the screen-space position of the sun's center
    gl_Position = u_m4_vp * (u_v4_eye_w+u_v4_light_position_w);
    // Perspective divide
    gl_Position /= gl_Position.w;
    // Move vertex in screen-space directly
    gl_Position.xy += a_position.xy * vec2(0.2, u_v4_framebuffer_size.z*0.2);
    // Draw far
    gl_Position.z = 0.999999f;

	v_uv = 2.f*a_uv-1.f;
}


#type fragment
#version 460 core
#include "include/forward_ubos.glsl"

layout(location = 0) in vec2 v_uv; // Texture coordinates

layout(location = 0) out vec4 out_color;

layout(std140, binding = 2) uniform material_data
{
    vec4 u_v4_sun_color;
};

float circle(in vec2 v2_uv, in float radius, in float blur)
{
    return 1.f-smoothstep(radius-(radius*blur),
                          radius+(radius*blur),
                          dot(v2_uv,v2_uv)*4.f);
}

float sun(in vec2 v2_uv, in float radius, in float blur)
{
    float intensity = circle(v2_uv, radius, blur);
    return intensity*intensity;
}


void main()
{
    out_color = u_v4_sun_color*sun(v_uv, 2.f, 1.f) + vec4(1.f)*sun(v_uv, 0.5f, 1.f);
}