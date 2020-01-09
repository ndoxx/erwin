#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) out vec2 v_uv;

void main()
{
	gl_Position = vec4(in_position, 1.f);
	v_uv = in_uv;
}

#type fragment
#version 460 core
#include "include/common.glsl"
#include "include/post_proc_inc.glsl"
#include "include/fxaa.glsl"

layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

SAMPLER_2D_(0);

layout(std140, binding = 0) uniform post_proc_layout
{
	vec4 u_vib_balance;     // Vibrance
	vec4 u_cor_gamma;       // Color correction
	float u_ca_shift;       // Chromatic aberration
	float u_ca_strength;    // Chromatic aberration
	float u_tm_exposure;    // Exposure tone mapping
	float u_vib_strength;   // Vibrance
	float u_cor_saturation; // Color correction
	float u_cor_contrast;   // Color correction
	
	vec2 u_fb_size;         // Framebuffer size
	int u_flags;			// Flags to enable/disable post-processing features
};

#define PP_EN_CHROMATIC_ABERRATION  1
#define PP_EN_EXPOSURE_TONE_MAPPING 2
#define PP_EN_VIBRANCE              4
#define PP_EN_SATURATION            8
#define PP_EN_CONTRAST              16
#define PP_EN_GAMMA                 32
#define PP_EN_FXAA                  64

void main()
{
    vec4 in_hdr = vec4(0.f);

	//  FXAA
    if(bool(u_flags & PP_EN_FXAA))
        in_hdr = FXAA(SAMPLER_2D_0, v_uv, u_fb_size);
    else
        in_hdr = texture(SAMPLER_2D_0, v_uv);

	vec3 color = in_hdr.rgb;

	// Chromatic aberration
    if(bool(u_flags & PP_EN_CHROMATIC_ABERRATION))
    	color = mix(color,chromatic_aberration_rgb(SAMPLER_2D_0, v_uv, u_fb_size, u_ca_shift, u_ca_strength),0.5f);

    // Tone mapping
    if(bool(u_flags & PP_EN_EXPOSURE_TONE_MAPPING))
    	color = exposure_tone_mapping_rgb(color, u_tm_exposure);
    else
    	color = reinhard_tone_mapping_rgb(color);

    // Vibrance
    if(bool(u_flags & PP_EN_VIBRANCE))
    	color = vibrance_rgb(color, u_vib_balance.rgb, u_vib_strength);

    // Color correction
    if(bool(u_flags & PP_EN_SATURATION))
    	color = saturate_rgb(color, max(0.0f, u_cor_saturation));
    if(bool(u_flags & PP_EN_CONTRAST))
    	color = contrast_rgb(color, u_cor_contrast);
    if(bool(u_flags & PP_EN_GAMMA))
    	color = gamma_correct_rgb(color, u_cor_gamma.rgb);

    out_color = vec4(color, in_hdr.a);
}