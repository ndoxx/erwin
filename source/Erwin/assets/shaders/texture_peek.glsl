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

layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform sampler2D us_input;

layout(std140, binding = 1) uniform peek_layout
{
	int u_flags;
	float u_split_pos;
	vec2 u_texel_size;
	vec4 u_channel_filter;
};

#define FLAG_TONE_MAP     1
#define FLAG_SPLIT_ALPHA  2
#define FLAG_INVERT       4

/*void main()
{
	out_color = texture(us_input, v_uv);
}*/

void main()
{
    /*if(b_isDepth)
    {
        float linearDepth = depth_view_from_tex(us_input, texCoord, v4_proj_params.zw);
        float depthRemapped = linearDepth / (linearDepth + 1.0);
        out_color = vec4(depthRemapped,depthRemapped,depthRemapped,1.f);
    }
    else
    {*/
        // "screen" texture is a floating point color buffer
        vec4 sampleColor = texture(us_input, v_uv);

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
    //}
}