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
    const vec2 vert_data[4] = vec2[]( vec2(-1.f, 1.f), vec2(-1.f, -1.f), vec2(1.f, 1.f), vec2(1.f, -1.f) );

    for(int ii=0; ii<4; ++ii)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = vec4(vert_data[ii].xy,0.f,1.f);
        EmitVertex();
    }

    EndPrimitive();
}


#type fragment
#version 460 core
#include "engine/common.glsl"

layout(location = 0) out vec4 out_color;

SAMPLER_2D_(0); // Equirectangular HDR texture

layout(std140, binding = 0) uniform parameters
{
	vec2 u_v2_viewport_size;
};

vec3 to_world_vector(vec2 uv, int face_idx)
{
	//                                  +x           -x           +y            -y          +z              -z
	//                                right         left          top         bottom       back           front
	const vec3 center[6] = vec3[]( vec3(1,0,0), vec3(-1,0,0), vec3(0,1,0),  vec3(0,-1,0), vec3(0,0,1),  vec3(0,0,-1) );
	const vec3 u_w[6]    = vec3[]( vec3(0,0,1), vec3(0,0,-1), vec3(-1,0,0), vec3(-1,0,0), vec3(-1,0,0), vec3(1,0,0) );
	const vec3 v_w[6]    = vec3[]( vec3(0,1,0), vec3(0,1,0),  vec3(0,0,-1), vec3(0,0,1),  vec3(0,1,0),  vec3(0,1,0) );

	return normalize(center[face_idx] - uv.x * u_w[face_idx] - uv.y * v_w[face_idx]);
}

vec2 sample_spherical_map(vec3 v)
{
	const vec2 inv_atan = vec2(0.1591f, 0.3183f);
	
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv = uv * inv_atan + 0.5f;
    return uv;
}

void main()
{
	// float intensity = float(gl_Layer+1)/6.f;
	// out_color = vec4(intensity,0.5f,1.f-intensity,1.f);

	vec2 uv = (gl_FragCoord.xy/u_v2_viewport_size)*2.f-1.f;
	vec3 pos_w = to_world_vector(uv, gl_Layer);
    vec2 s_uv = sample_spherical_map(pos_w);
    vec3 color = texture(SAMPLER_2D_0, s_uv).rgb;

    // vec3 color = vec3(s_uv,0.f);
    // vec3 color = 0.5f*pos_w+0.5f;
    // vec3 color = pos_w;

    out_color = vec4(color, 1.f);
}