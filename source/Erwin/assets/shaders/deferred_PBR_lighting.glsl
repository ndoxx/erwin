#type vertex
#version 460 core
#include "engine/frame_ubo.glsl"
#include "engine/transform_ubo.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec2 v2_frag_ray_v;
layout(location = 2) out vec3 v3_light_dir_v;

void main()
{
    gl_Position = vec4(a_position, 1.f);
    v_uv = a_uv;

    vec4 clip_pos = vec4(a_position, 1.f);
    v2_frag_ray_v = clip_pos.xy / clip_pos.w;
    v2_frag_ray_v *= u_v4_proj_params.xy;

    // Light position in view space
    vec4 v4_light_pos_v = u_m4_v * vec4(u_v4_light_position_w.xyz, 0.f);
    v3_light_dir_v = normalize(v4_light_pos_v.xyz);
}


#type fragment
#version 460 core
#include "engine/common.glsl"
#include "engine/frame_ubo.glsl"
#include "engine/transform_ubo.glsl"
#include "engine/position.glsl"
#include "engine/glow.glsl"
#include "engine/normal_compression.glsl"
#include "engine/cook_torrance.glsl"

SAMPLER_2D_(0); // albedo
SAMPLER_2D_(1); // normal
SAMPLER_2D_(2); // metallic - ambient occlusion - roughness - emissivity
SAMPLER_2D_(3); // depth
SAMPLER_2D_(4); // BRDF integration map

SAMPLER_CUBE_(5); // irradiance cubemap

layout(location = 0) in vec2 v_uv;          // Texture coordinates
layout(location = 1) in vec2 v2_frag_ray_v;
layout(location = 2) in vec3 v3_light_dir_v;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_glow;

const float f_bright_threshold = 0.7f;
const float f_bright_knee = 0.1f;

void main()
{
	// Retrieve GBuffer data
	vec4 GBuffer_albedo = texture(SAMPLER_2D_0, v_uv);
    vec4 GBuffer_normal = texture(SAMPLER_2D_1, v_uv);
    vec4 GBuffer_mar    = texture(SAMPLER_2D_2, v_uv);
    float GBuffer_depth = texture(SAMPLER_2D_3, v_uv).r;

    // Reconstruct position from depth buffer
    vec3 frag_pos = reconstruct_position(GBuffer_depth, v2_frag_ray_v, u_v4_proj_params);

    // View direction as a vector from fragment position to camera position
    vec3 view_dir = normalize(-frag_pos);

	vec3 frag_albedo     = GBuffer_albedo.rgb;
	vec3 frag_normal     = decompress_normal_z_reconstruct(GBuffer_normal.xy);
	float frag_metallic  = GBuffer_mar.x;
	float frag_ao        = GBuffer_mar.y;
	float frag_roughness = GBuffer_mar.z;
    float frag_emissive  = GBuffer_albedo.a;

	// Apply BRDF
    vec3 radiance = CookTorrance(u_v4_light_color.rgb,
                                 v3_light_dir_v,
                                 frag_normal,
                                 view_dir,
                                 frag_albedo,
                                 frag_metallic,
                                 frag_roughness);

    vec3 ambient;
    if(bool(u_i_frame_flags & FRAME_FLAG_ENABLE_DIFFUSE_IBL))
    {
        // Ambient IBL
        // Convert normal from view space to world space
        vec3 normal_w = (u_m4_Tv * vec4(frag_normal, 0.f)).xyz;
        vec3 F0 = mix(vec3(0.04f), frag_albedo, frag_metallic);
        vec3 kS = FresnelSchlickRoughness(max(dot(frag_normal, view_dir), 0.f), F0, frag_roughness);
        vec3 kD = 1.f - kS;
        vec3 irradiance = texture(SAMPLER_CUBE_5, normal_w).rgb;
        vec3 diffuse    = irradiance * frag_albedo;
        ambient         = (kD * diffuse) * frag_ao * u_f_light_ambient_strength;
    }
    else
    {
        // Uniform ambient term
        ambient = (frag_ao * u_f_light_ambient_strength) * frag_albedo * u_v4_light_ambient_color.rgb;
    }

    vec3 total_light = radiance + ambient;

    total_light += frag_emissive * frag_albedo;

    out_color = vec4(total_light, 1.f);

    // "Bright pass"
    out_glow = glow(out_color.rgb, f_bright_threshold, f_bright_knee);
}