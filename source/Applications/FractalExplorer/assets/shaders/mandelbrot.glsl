// Code adapted from The Art of Code: https://www.youtube.com/watch?v=zmWkhlocBRY

#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 v_uv;

uniform mat4 u_view_projection;

void main()
{
    gl_Position = vec4(in_position, 1.f);
  	v_uv = (u_view_projection*vec4(in_uv,0.f,1.f)).xy;
}

#type fragment
#version 460 core

in vec2 v_uv;

layout(location = 0) out vec4 out_color;

uniform float u_max_iter = 20;
uniform float u_escape_radius = 2.f;
uniform float u_time = 0.f;
uniform float u_atten = 1.f;
uniform vec3 u_palette = vec3(0.3f,0.7f,0.9f);

vec2 rot(vec2 point, vec2 pivot, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    point -= pivot;
    point  = vec2(c*point.x-s*point.y, s*point.x+c*point.y);
    point += pivot;

    return point;
}

void main()
{
    vec2 c = 2.f*v_uv-1.f;

    float r2 = u_escape_radius*u_escape_radius;

    vec2 z, z_prev;
    float iter = 0.f;
    for(; iter<u_max_iter; ++iter)
    {
        // z_prev = z;
        z_prev = rot(z, vec2(0.f), u_time);
        z = vec2(z.x*z.x-z.y*z.y, 2*z.x*z.y) + c;

        if(dot(z,z_prev)>r2) break;
    }
    if(iter>u_max_iter)
    {
        out_color = vec4(0.f,0.f,0.f,1.f);
        return;
    }

    iter /= u_max_iter;

    float dist = length(z);
    float frac_iter = log2(u_atten*log(dist)/log(u_escape_radius));

    float m = sqrt(iter);
    vec4 col = 0.5f*(sin(vec4(u_palette,1.f)*m*20.f)+1.f);
    col *= smoothstep(3.f,0.f,frac_iter);

	out_color = col;
}
