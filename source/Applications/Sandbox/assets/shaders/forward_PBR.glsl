#type vertex
#version 460 core
#include "include/tangent.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;          // Texture coordinates
layout(location = 1) out vec3 v_view_dir_v;  // Vertex view direction, view space
layout(location = 2) out vec3 v_view_dir_t;  // Vertex view direction, tangent space
layout(location = 3) out vec3 v_light_dir_v; // Light direction, view space
layout(location = 4) out mat3 v_TBN;         // TBN matrix for normal mapping

layout(std140, binding = 0) uniform pass_data
{
	mat4 u_m4_v;   // view
	mat4 u_m4_vp;  // view-projection

	vec4 u_v4_light_position_w; // Directional light position, world space
	vec4 u_v4_light_color;
	float u_f_light_ambient_strength;
};
layout(std140, binding = 1) uniform instance_data
{
	mat4 u_m4_m;    // model
	mat4 u_m4_mv;   // model-view
	mat4 u_m4_mvp;  // model-view-projection
};
layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_tint; // tint
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
}



#type fragment
#version 460 core
#include "include/common.glsl"
#include "include/cook_torrance.glsl"
#include "include/parallax.glsl"

SAMPLER_2D_(0); // albedo
SAMPLER_2D_(1); // normal - depth
SAMPLER_2D_(2); // metallic - roughness - ambient occlusion

layout(location = 0) in vec2 v_uv;          // Texture coordinates
layout(location = 1) in vec3 v_view_dir_v;  // Vertex view direction, view space
layout(location = 2) in vec3 v_view_dir_t;  // Vertex view direction, tangent space
layout(location = 3) in vec3 v_light_dir_v; // Light direction, view space
layout(location = 4) in mat3 v_TBN;         // TBN matrix for normal mapping

layout(location = 0) out vec4 out_color;

layout(std140, binding = 0) uniform pass_data
{
	mat4 u_m4_v;   // view
	mat4 u_m4_vp;  // view-projection

	vec4 u_v4_light_position_w; // Directional light position, world space
	vec4 u_v4_light_color;
	float u_f_light_ambient_strength;
};
layout(std140, binding = 1) uniform instance_data
{
	mat4 u_m4_m;    // model
	mat4 u_m4_mv;   // model-view
	mat4 u_m4_mvp;  // model-view-projection
};
layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_tint; // tint
};

const float f_parallax_height_scale = 0.05f;

void main()
{
	vec2 tex_coord = v_uv;

	// Parallax map
	// vec2 tex_coord = parallax_map(v_uv, v_view_dir_t, f_parallax_height_scale, SAMPLER_2D_1);

	// Retrieve texture data
	vec4 frag_color = texture(SAMPLER_2D_0, tex_coord);
	vec3 frag_albedo = frag_color.rgb * u_v4_tint.rgb;
	float frag_alpha = frag_color.a;
	vec3 frag_normal = v_TBN*normalize(texture(SAMPLER_2D_1, tex_coord).xyz * 2.f - 1.f);
	vec3 frag_mra    = texture(SAMPLER_2D_2, tex_coord).xyz;
	float frag_metallic  = frag_mra.x;
	float frag_roughness = frag_mra.y;
	float frag_ao        = frag_mra.z;

	// Apply BRDF
    vec3 radiance = CookTorrance(u_v4_light_color.rgb,
                                 v_light_dir_v,
                                 frag_normal,
                                 v_view_dir_v,
                                 frag_albedo,
                                 frag_metallic,
                                 frag_roughness);
    vec3 ambient = (frag_ao * u_f_light_ambient_strength) * frag_albedo;
    vec3 total_light = radiance + ambient;

    out_color = vec4(total_light,frag_alpha);
}