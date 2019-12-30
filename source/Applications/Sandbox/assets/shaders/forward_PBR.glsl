#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) out vec2 v_uv;
layout(location = 5) out vec3 v_view_dir;
layout(location = 6) out vec3 v_tangent_view_dir;
layout(location = 7) out vec3 v_light_dir;
layout(location = 8) out vec3 v_frag_pos;
layout(location = 9) out mat3 v_TBN;

layout(std140, binding = 0) uniform instance_data
{
	mat4 u_mvp;  // model-view-projection
	mat4 u_mv;   // model-view
	mat4 u_m;    // model
	vec4 u_tint; // tint
};
layout(std140, binding = 1) uniform pass_data
{
	mat4 u_vp;  // view-projection
	mat4 u_v;   // view
};

const vec3 light_position = normalize(vec3(0.f,0.3f,-1.f));

void main()
{
	gl_Position = u_mvp*vec4(in_position, 1.f);

	// Compute TBN matrix
	mat3 m3_normal = mat3(u_mv);
    vec3 N = normalize(m3_normal * in_normal);
    vec3 T = normalize(m3_normal * in_tangent);
    // Re-orthogonalize T with respect to N by the Gram-Schmidt process
    T = normalize(T - dot(T, N)*N);
    vec3 B = -cross(N, T); // Minus sign might be needed for normal/parallax mapping to work correctly
    v_TBN = mat3(T,B,N);
    mat3 TBN_inv = transpose(v_TBN);

    vec4 view_pos = u_mv*vec4(in_position, 1.f);
    vec4 light_pos = u_mv*vec4(light_position, 0.f);
    vec3 vertex_pos = -view_pos.xyz/view_pos.w;
    v_view_dir = normalize(vertex_pos);
    v_tangent_view_dir = normalize(TBN_inv * vertex_pos);
    v_light_dir = normalize(light_pos.xyz);
    v_frag_pos = vec3(u_m*vec4(in_position, 1.f));
	v_uv = in_uv;
}

#type fragment
#version 460 core
#include "include/common.glsl"
#include "include/cook_torrance.glsl"
#include "include/parallax.glsl"

layout(location = 4) in vec2 v_uv;
layout(location = 5) in vec3 v_view_dir;
layout(location = 6) in vec3 v_tangent_view_dir;
layout(location = 7) in vec3 v_light_dir;
layout(location = 8) in vec3 v_frag_pos;
layout(location = 9) in mat3 v_TBN;
layout(location = 0) out vec4 out_color;
SAMPLER_2D_(0); // albedo
SAMPLER_2D_(1); // normal - depth
SAMPLER_2D_(2); // metallic - roughness - ambient occlusion

layout(std140, binding = 0) uniform instance_data
{
	mat4 u_mvp;  // model-view-projection
	mat4 u_mv;   // model-view
	mat4 u_m;    // model
	vec4 u_tint; // tint
};
layout(std140, binding = 1) uniform pass_data
{
	mat4 u_vp;  // view-projection
	mat4 u_v;   // view
};

const vec3 light_color = vec3(0.95f,0.85f,0.5f);
const float f_ambient_strength = 0.1f;
const float f_parallax_height_scale = 0.1f;

void main()
{
	// Parallax map
	// vec2 tex_coord = v_uv;
	vec2 tex_coord = parallax_map(v_uv, v_tangent_view_dir, f_parallax_height_scale, SAMPLER_2D_1);

	vec4 frag_color = texture(SAMPLER_2D_0, tex_coord);
	vec3 frag_albedo = frag_color.rgb * u_tint.rgb;
	float frag_alpha = frag_color.a;
	vec3 frag_normal = v_TBN*normalize(texture(SAMPLER_2D_1, tex_coord).xyz * 2.f - 1.f);
	vec3 frag_mra    = texture(SAMPLER_2D_2, tex_coord).xyz;
	float frag_metallic  = frag_mra.x;
	float frag_roughness = frag_mra.y;
	float frag_ao        = frag_mra.z;
    vec3 radiance = CookTorrance(light_color,
                                 v_light_dir,
                                 frag_normal,
                                 v_view_dir,
                                 frag_albedo,
                                 frag_metallic,
                                 frag_roughness);
    vec3 ambient = (frag_ao * f_ambient_strength) * frag_albedo;
    vec3 total_light = radiance + ambient;

    out_color = vec4(total_light,frag_alpha);
    // out_color = vec4(v_tangent_view_dir, 1.f);
}