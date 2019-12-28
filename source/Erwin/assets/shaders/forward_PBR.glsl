#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_view_dir;
layout(location = 4) out vec3 v_frag_pos;

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

void main()
{
	gl_Position = u_mvp*vec4(in_position, 1.f);

    vec4 view_pos = u_mv*vec4(in_position, 1.f);
    v_view_dir = normalize(-view_pos.xyz/view_pos.w);
    v_frag_pos = vec3(u_m*vec4(in_position, 1.f));
	v_uv = in_uv;
}

#type fragment
#version 460 core
#include "include/common.glsl"
#include "include/cook_torrance.glsl"

layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_view_dir;
layout(location = 4) in vec3 v_frag_pos;
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
const vec3 light_position = normalize(vec3(1.f,1.f,1.f));
const float ambient_strength = 0.1f;

void main()
{
	vec4 frag_color = texture(SAMPLER_2D_0, v_uv);
	vec3 frag_albedo = frag_color.rgb * u_tint.rgb;
	float frag_alpha = frag_color.a;
	// TODO: transform normal using TBN matrix, for that we need normal and tangent attributes -> proper mesh system
	/*vec3 frag_normal = normalize(texture(SAMPLER_2D_1, v_uv).xyz * 2.f - 1.f);
	vec3 frag_mra    = texture(SAMPLER_2D_2, v_uv).xyz;
	float frag_metallic  = frag_mra.x;
	float frag_roughness = frag_mra.y;
	float frag_ao        = frag_mra.z;
	vec3 light_dir = normalize(light_position - v_frag_pos);
    vec3 radiance = CookTorrance(light_color,
                                 light_dir,
                                 frag_normal,
                                 v_view_dir,
                                 frag_albedo,
                                 frag_metallic,
                                 frag_roughness);
    vec3 ambient = (frag_ao * ambient_strength) * frag_albedo;
    vec3 total_light = radiance + ambient;

    out_color = vec4(total_light,frag_alpha);*/
    out_color = vec4(frag_albedo,frag_alpha);
}