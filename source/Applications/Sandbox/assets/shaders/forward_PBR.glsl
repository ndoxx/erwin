#type vertex
#version 460 core
#include "engine/tangent.glsl"
#include "engine/forward_ubos.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;          // Texture coordinates
layout(location = 1) out vec3 v_view_dir_v;  // Vertex view direction, view space
layout(location = 2) out vec3 v_view_dir_t;  // Vertex view direction, tangent space
layout(location = 3) out vec3 v_light_dir_v; // Light direction, view space
layout(location = 4) out mat3 v_TBN;         // TBN matrix for normal mapping

layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_tint; // tint
	int u_flags;
	float u_f_emissive_scale;
};

void main()
{
	gl_Position = u_m4_mvp*vec4(a_position, 1.f);

	// Compute TBN matrix for normal mapping
	// We assume uniform scaling, so no need to transpose-inverse the model-view matrix
    v_TBN = TBN(mat3(u_m4_mv), a_normal, a_tangent);
    mat3 TBN_inv = transpose(v_TBN);

    // Light position, view space
    vec4 light_pos_v = u_m4_v*vec4(u_v4_light_position_w.xyz, 0.f);
    // Vertex position, view space
    vec4 vertex_pos_v = u_m4_mv*vec4(a_position, 1.f);
    
    v_view_dir_v = normalize(-vertex_pos_v.xyz/vertex_pos_v.w);
    v_view_dir_t = normalize(TBN_inv * v_view_dir_v);
    // light direction = position for directional light
    v_light_dir_v = normalize(light_pos_v.xyz);
	v_uv = a_uv;
}



#type fragment
#version 460 core
#include "engine/common.glsl"
#include "engine/glow.glsl"
#include "engine/cook_torrance.glsl"
#include "engine/parallax.glsl"
#include "engine/forward_ubos.glsl"

#define PBR_EN_EMISSIVE 1

SAMPLER_2D_(0); // albedo
SAMPLER_2D_(1); // normal - depth
SAMPLER_2D_(2); // metallic - ambient occlusion - roughness - emissivity

layout(location = 0) in vec2 v_uv;          // Texture coordinates
layout(location = 1) in vec3 v_view_dir_v;  // Vertex view direction, view space
layout(location = 2) in vec3 v_view_dir_t;  // Vertex view direction, tangent space
layout(location = 3) in vec3 v_light_dir_v; // Light direction, view space
layout(location = 4) in mat3 v_TBN;         // TBN matrix for normal mapping

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_glow;

layout(std140, binding = 2) uniform material_data
{
	vec4 u_v4_tint; // tint
	int u_flags;
	float u_f_emissive_scale;
};

const float f_parallax_height_scale = 0.02f;
const float f_bright_threshold = 0.7f;
const float f_bright_knee = 0.1f;

const mat4 m4_stippling_threshold = mat4(vec4(1,13,4,16),vec4(9,5,12,8),vec4(3,15,2,14),vec4(11,7,10,6))/17.f;

void main()
{
    // Stippling: selectively discard fragments when surface is too close to eye position
    /*float threshold = m4_stippling_threshold[int(gl_FragCoord.x)%4][int(gl_FragCoord.y)%4];
    if(1.2f*gl_FragCoord.z-threshold < 0.f)
        discard;*/

	vec2 tex_coord = v_uv;

	// Parallax map
	// vec2 tex_coord = parallax_map(v_uv, v_view_dir_t, f_parallax_height_scale, SAMPLER_2D_1);

	// Retrieve texture data
	vec4 frag_color = texture(SAMPLER_2D_0, tex_coord);
	vec3 frag_albedo = frag_color.rgb * u_v4_tint.rgb;
	float frag_alpha = frag_color.a;
	vec3 frag_normal = v_TBN*normalize(texture(SAMPLER_2D_1, tex_coord).xyz * 2.f - 1.f);
	vec4 frag_mare    = texture(SAMPLER_2D_2, tex_coord);
	float frag_metallic  = frag_mare.x;
	float frag_ao        = frag_mare.y;
	float frag_roughness = frag_mare.z;
	float frag_emissive  = frag_mare.w;

	// Apply BRDF
    vec3 radiance = CookTorrance(u_v4_light_color.rgb,
                                 v_light_dir_v,
                                 frag_normal,
                                 v_view_dir_v,
                                 frag_albedo,
                                 frag_metallic,
                                 frag_roughness);
    vec3 ambient = (frag_ao * u_f_light_ambient_strength) * frag_albedo * u_v4_light_ambient_color.rgb;
    vec3 total_light = radiance + ambient;

    if(bool(u_flags & PBR_EN_EMISSIVE))
    {
    	total_light += u_f_emissive_scale * frag_emissive * frag_albedo;
    }

    out_color = vec4(total_light, frag_alpha);

    // "Bright pass"
    out_glow = glow(out_color.rgb, f_bright_threshold, f_bright_knee);
}