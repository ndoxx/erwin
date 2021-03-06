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
#include "engine/common.glsl"

layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec4 out_color;
SAMPLER_2D_(0);

layout(std140, binding = 1) uniform peek_layout
{
	int u_flags;
	float u_split_pos;
	vec2 u_texel_size;
	vec4 u_channel_filter;
	vec4 u_proj_params;
};

#define FLAG_TONE_MAP     1
#define FLAG_SPLIT_ALPHA  2
#define FLAG_INVERT       4
#define FLAG_DEPTH        8

float depth_view_from_tex(in sampler2D depthTex, in vec2 texCoords, in vec2 projParams)
{
    // Get screen space depth at coords
    float depth_raw = texture(depthTex, texCoords).r;
    // Convert to NDC depth (*2-1) and add P(2,2)
    float depth_ndc_offset = depth_raw * 2.0f + projParams.x;
    // Return positive linear depth
    return projParams.y / depth_ndc_offset;
}

void main()
{
    if(bool(u_flags & FLAG_DEPTH))
    {
        float linearDepth = depth_view_from_tex(SAMPLER_2D_0, v_uv, u_proj_params.zw);
        float depthRemapped = linearDepth / (linearDepth + 1.0);
        out_color = vec4(depthRemapped,depthRemapped,depthRemapped,1.f);
    }
    else
    {
        // "screen" texture is a floating point color buffer
        vec4 sampleColor = texture(SAMPLER_2D_0, v_uv);

        // Filter channels
        sampleColor.rgb *= u_channel_filter.rgb;

        // If single channel, display in gray levels
        if(dot(u_channel_filter.rgb, vec3(1.f))<2.f)
        {
            float monochrome = dot(sampleColor.rgb, u_channel_filter.rgb);
            sampleColor.rgb = vec3(monochrome);
        }

        // Fast Reinhard tone mapping
    	if(bool(u_flags & FLAG_TONE_MAP))
        	sampleColor = sampleColor / (sampleColor + vec4(1.0));

        // Color inversion
    	if(bool(u_flags & FLAG_INVERT))
        	sampleColor.rgb = 1.f - sampleColor.rgb;

        if(bool(u_flags & FLAG_SPLIT_ALPHA) && v_uv.x>u_split_pos)
        {
            if(v_uv.x<u_split_pos+1.01f*u_texel_size.x)
                sampleColor.rgb = vec3(1.0,0.5,0.0);
            else
                sampleColor.rgb = vec3(sampleColor.a);
        }

        out_color = vec4(sampleColor.rgb, 1.f);
    }
}