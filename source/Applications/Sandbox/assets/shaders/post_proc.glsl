#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 v_uv;

void main()
{
	gl_Position = vec4(in_position, 1.f);
	v_uv = in_uv;
}

#type fragment
#version 460 core

#include include/post_proc_inc.glsl

in vec2 v_uv;
layout(location = 0) out vec4 out_color;

uniform sampler2D us_input;

struct PostProcData
{
	// Chromatic aberration
	float ca_shift;
	float ca_strength;
	// Vibrance
	// float vib_strength;
	// vec3 vib_balance;

	vec2 fb_size;
};

layout(std140) uniform post_proc_layout
{
    PostProcData pp_data;
};

void main()
{
	vec4 in_hdr = texture(us_input, v_uv);
	vec3 color_hdr = in_hdr.rgb;

    color_hdr = chromatic_aberration_rgb(us_input, v_uv, pp_data.fb_size, pp_data.ca_shift, pp_data.ca_strength);

    // color_hdr = vibrance_rgb(color_hdr, pp_data.vib_balance, pp_data.vib_strength);

    out_color = vec4(color_hdr, in_hdr.a);
}