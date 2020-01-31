#type vertex
#version 460 core
#include "engine/frame_ubo.glsl"
#include "engine/transform_ubo.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv; // Texture coordinates

layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_sun_color;
    float u_f_sun_scale;
    float u_f_sun_brightness;
};

void main()
{
    // z component always 1 after perspective divide
    // gl_Position = pos.xyww;

    // * Simple billboarding routine: draw a fixed size screen facing quad at the sun's position
    float far = u_v4_camera_params.y;
    // Get the screen-space position of the sun's center
    gl_Position = u_m4_vp * (u_v4_eye_w+u_v4_light_position_w*far);
    // Perspective divide
    gl_Position /= gl_Position.w;
    // Move vertex in screen-space directly
    // float scale = u_f_sun_scale*smoothstep(0.1f,5.f,u_f_sun_brightness);
    // gl_Position.xy += a_position.xy * vec2(scale, u_v4_framebuffer_size.z*scale);
    gl_Position.xy += a_position.xy * vec2(u_f_sun_scale, u_v4_framebuffer_size.z*u_f_sun_scale);

	v_uv = 2.f*a_uv-1.f;
}


#type fragment
#version 460 core
#include "engine/frame_ubo.glsl"
#include "engine/transform_ubo.glsl"

layout(location = 0) in vec2 v_uv; // Texture coordinates

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_glow;

layout(std140, binding = 2) uniform material_data
{
    vec4 u_v4_sun_color;
    float u_f_sun_scale;
    float u_f_sun_brightness;
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

// Relative luminance coefficients for sRGB primaries, values from Wikipedia
const vec3 W = vec3(0.2126f, 0.7152f, 0.0722f);
const float f_bright_threshold = 0.7f;
const float f_bright_knee = 0.1f;

void main()
{
    // float center_sat = smoothstep(0.5f,10.f,u_f_sun_brightness);
    // float halo = smoothstep(0.01f,1.f,u_f_sun_brightness);
    // out_color = halo*u_v4_sun_color*sun(v_uv, 2.f, 1.f) + vec4(5.f*center_sat)*sun(v_uv, 0.5f*center_sat, 1.f);
    out_color = u_v4_sun_color*sun(v_uv, 2.f, 1.f) 
              + mix(u_v4_sun_color,vec4(1.0f),0.5f)*sun(v_uv, 0.5f, 1.f) 
              + vec4(1.f)*sun(v_uv, 0.3f, 1.f);

    // Sun already has a halo, no need for bloom effect on top of it
    out_glow = vec4(0.f);
}