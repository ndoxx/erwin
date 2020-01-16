#type vertex
#version 460 core
#include "engine/forward_ubos.glsl" // TODO: Change name

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
#include "engine/forward_ubos.glsl" // TODO: Change name
#include "engine/position.glsl"
#include "engine/glow.glsl"
#include "engine/normal_compression.glsl"
#include "engine/cook_torrance.glsl"

SAMPLER_2D_(0); // albedo
SAMPLER_2D_(1); // normal
SAMPLER_2D_(2); // metallic - ambient occlusion - roughness - emissivity
SAMPLER_2D_(3); // depth

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
    vec4 GBuffer_mare   = texture(SAMPLER_2D_2, v_uv);
    float GBuffer_depth = texture(SAMPLER_2D_3, v_uv).r;

    // Reconstruct position from depth buffer
    vec3 frag_pos = reconstruct_position(GBuffer_depth, v2_frag_ray_v, u_v4_proj_params);

    // View direction as a vector from fragment position to camera position
    vec3 view_dir = normalize(-frag_pos);

	vec3 frag_albedo     = GBuffer_albedo.rgb;
	float frag_alpha     = GBuffer_albedo.a;
	vec3 frag_normal     = decompress_normal_spheremap_transform(GBuffer_normal.xy);
	float frag_metallic  = GBuffer_mare.x;
	float frag_ao        = GBuffer_mare.y;
	float frag_roughness = GBuffer_mare.z;
	float frag_emissive  = GBuffer_mare.w;

	// Apply BRDF
    vec3 radiance = CookTorrance(u_v4_light_color.rgb,
                                 v3_light_dir_v,
                                 frag_normal,
                                 view_dir,
                                 frag_albedo,
                                 frag_metallic,
                                 frag_roughness);
    vec3 ambient = (frag_ao * u_f_light_ambient_strength) * frag_albedo * u_v4_light_ambient_color.rgb;
    vec3 total_light = radiance + ambient;

    total_light += frag_emissive * frag_albedo;

    out_color = vec4(total_light, frag_alpha);

    // "Bright pass"
    out_glow = glow(out_color.rgb, f_bright_threshold, f_bright_knee);
}