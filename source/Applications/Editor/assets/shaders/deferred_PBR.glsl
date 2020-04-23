#type vertex
#version 460 core
#include "engine/tangent.glsl"
#include "engine/frame_ubo.glsl"
#include "engine/transform_ubo.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;          // Texture coordinates
layout(location = 1) out vec3 v_normal;      // Vertex normal
layout(location = 2) out vec3 v_view_dir_v;  // Vertex view direction, view space
layout(location = 3) out vec3 v_view_dir_t;  // Vertex view direction, tangent space
layout(location = 4) out vec3 v_light_dir_v; // Light direction, view space
layout(location = 5) out mat3 v_TBN;         // TBN matrix for normal mapping

layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_tint;
	int u_flags;
	float u_f_emissive_scale;
    float u_f_tiling_factor;
    float u_f_parallax_height_scale;

    // Uniform maps
    vec4 u_v4_uniform_albedo;
    float u_f_uniform_metallic;
    float u_f_uniform_roughness;
};

void main()
{
	gl_Position = u_m4_mvp*vec4(a_position, 1.f);

	// Compute TBN matrix for normal mapping
	// We assume uniform scaling, so no need to transpose-inverse the model-view matrix
    v_TBN = TBN(mat3(u_m4_mv), a_normal, a_tangent);
    mat3 TBN_inv = transpose(v_TBN);

    // Light position, view space
    vec4 light_pos_v = u_m4_v*vec4(-u_v4_light_position_w.xyz, 0.f);
    // Vertex position, view space
    vec4 vertex_pos_v = u_m4_mv*vec4(a_position, 1.f);
    
    v_view_dir_v = normalize(-vertex_pos_v.xyz/vertex_pos_v.w);
    v_view_dir_t = normalize(TBN_inv * v_view_dir_v);
    // light direction = position for directional light
    v_light_dir_v = normalize(-light_pos_v.xyz);
	v_uv = a_uv;
    v_normal = normalize(mat3(u_m4_mv)*a_normal);
}



#type fragment
#version 460 core
#include "engine/common.glsl"
#include "engine/parallax.glsl"
#include "engine/normal_compression.glsl"
#include "engine/frame_ubo.glsl"
#include "engine/transform_ubo.glsl"

#define PBR_EN_EMISSIVE      1<<0
#define PBR_EN_PARALLAX      1<<1
#define PBR_EN_ALBEDO_MAP    1<<2
#define PBR_EN_NORMAL_MAP    1<<3
#define PBR_EN_METALLIC_MAP  1<<4
#define PBR_EN_AO_MAP        1<<5
#define PBR_EN_ROUGHNESS_MAP 1<<6

SAMPLER_2D_(0); // albedo
SAMPLER_2D_(1); // normal - depth
SAMPLER_2D_(2); // metallic - ambient occlusion - roughness

layout(location = 0) in vec2 v_uv;          // Texture coordinates
layout(location = 1) in vec3 v_normal;      // Vertex normal
layout(location = 2) in vec3 v_view_dir_v;  // Vertex view direction, view space
layout(location = 3) in vec3 v_view_dir_t;  // Vertex view direction, tangent space
layout(location = 4) in vec3 v_light_dir_v; // Light direction, view space
layout(location = 5) in mat3 v_TBN;         // TBN matrix for normal mapping

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_mar;

layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_tint;
	int u_flags;
	float u_f_emissive_scale;
    float u_f_tiling_factor;
    float u_f_parallax_height_scale;

    // Uniform maps
    vec4 u_v4_uniform_albedo;
    float u_f_uniform_metallic;
    float u_f_uniform_roughness;
};

void main()
{
	vec2 tex_coord = u_f_tiling_factor*v_uv;

	// Parallax map
    if(bool(u_flags & PBR_EN_PARALLAX))
	   tex_coord = parallax_map(tex_coord, v_view_dir_t, u_f_parallax_height_scale, SAMPLER_2D_1);

	// Retrieve texture data
    vec4 frag_color;
    if(bool(u_flags & PBR_EN_ALBEDO_MAP))
        frag_color = texture(SAMPLER_2D_0, tex_coord);
    else
        frag_color = u_v4_uniform_albedo;

    vec3 frag_normal;
    if(bool(u_flags & PBR_EN_NORMAL_MAP))
        frag_normal = v_TBN*normalize(texture(SAMPLER_2D_1, tex_coord).xyz * 2.f - 1.f);
    else
        frag_normal = v_normal;

    // Compress normal
    vec2 normal_cmp = compress_normal_z_reconstruct(frag_normal);

    vec4 frag_mare_tex = texture(SAMPLER_2D_2, tex_coord);
    vec3 frag_mar;
    frag_mar.r  = bool(u_flags & PBR_EN_METALLIC_MAP) ? frag_mare_tex.r : u_f_uniform_metallic;
    frag_mar.g  = bool(u_flags & PBR_EN_AO_MAP) ? frag_mare_tex.g : 1.f;
    frag_mar.b  = bool(u_flags & PBR_EN_ROUGHNESS_MAP) ? frag_mare_tex.b : u_f_uniform_roughness;
    float alpha = bool(u_flags & PBR_EN_EMISSIVE) ? frag_mare_tex.a * u_f_emissive_scale : 0.f;
    
    out_albedo = vec4(frag_color.rgb * u_v4_tint.rgb, alpha);
    out_normal = vec4(normal_cmp, 0.f, 1.f);
    out_mar    = vec4(frag_mar, 1.f);
}