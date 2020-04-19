#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

void main()
{
    gl_Position = vec4(a_position, 1.f);
}


#type geometry
#version 460 core

layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 4) out;

void main() 
{     
    const vec2 vert_data[4] = { vec2(-1.f, 1.f), vec2(-1.f, -1.f), vec2(1.f, 1.f), vec2(1.f, -1.f) };

    for(int ii=0; ii<4; ++ii)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = vec4(vert_data[ii],0.f,1.f);
        EmitVertex();
    }

    EndPrimitive();
}


#type fragment
#version 460 core
#include "engine/common.glsl"
#include "engine/sphere_map.glsl"

layout(location = 0) out vec4 out_color;

SAMPLER_CUBE_(0); // Environment map

layout(std140, binding = 0) uniform parameters
{
    vec2 u_v2_viewport_size;
    float u_f_delta;
};

const float PI = 3.14159265359;

void main()
{
    vec2 uv       = (gl_FragCoord.xy/u_v2_viewport_size)*2.f-1.f;
    vec3 view_dir = to_world_vector(uv, gl_Layer);
    vec3 up    = vec3(0.f, 1.f, 0.f);
    vec3 right = cross(up, view_dir);
    up         = cross(view_dir, right);

    vec3 irradiance = vec3(0.f);
    float sample_count = 0.f; 
    for(float theta=0.f; theta<0.5f*PI; theta+=u_f_delta)
    {
        float sin_theta = sin(theta);
        float cos_theta = cos(theta);
        for(float phi=0.f; phi<2.f*PI; phi+=u_f_delta)
        {
            // spherical to cartesian (in tangent space)
            vec3 sample_t = vec3(sin_theta*cos(phi), sin_theta*sin(phi), cos_theta);
            // tangent space to world
            vec3 sample_w = sample_t.x * right + (sample_t.y * up + (sample_t.z * view_dir));

            irradiance += texture(SAMPLER_CUBE_0, sample_w).rgb * cos_theta * sin_theta;
            sample_count += 1.f;
        }
    }
    irradiance = PI * irradiance / sample_count;

    // HACK? tonemap and gamma correction on output irradiance avoids weird diffuse IBL reflections
    irradiance = irradiance / (irradiance + vec3(1.f));
    irradiance = pow(irradiance, vec3(1.f/2.2f));

    out_color = vec4(irradiance, 1.f);
}