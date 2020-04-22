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
#include "engine/importance_sampling.glsl"

layout(location = 0) out vec4 out_color;

SAMPLER_CUBE_(0); // Environment map

layout(std140, binding = 0) uniform parameters
{
    vec2 u_v2_viewport_size;
    float u_f_roughness;
    float u_f_source_resolution;
};

void main()
{
    vec2 uv       = (gl_FragCoord.xy/u_v2_viewport_size)*2.f-1.f;
    vec3 view_dir = to_world_vector(uv, gl_Layer);

    vec3 N = normalize(view_dir);    
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.f;   
    vec3 prefilteredColor = vec3(0.f);     
    for(uint ii=0u; ii<SAMPLE_COUNT; ++ii)
    {
        vec2 Xi = Hammersley(ii, SAMPLE_COUNT);
        vec3 H  = importance_sample_GGX(Xi, N, u_f_roughness);
        vec3 L  = normalize(2.f * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.f);
        if(NdotL > 0.f)
        {
            // sample from the environment's mip level based on roughness/pdf
            // avoids bright dots artifacts near bright areas for high roughness mips 
            float NdotH = max(dot(N, H), 0.f);
            float HdotV = max(dot(H, V), 0.f);
            float D   = TrowbridgeReitzGGX(NdotH, u_f_roughness);
            float pdf = D * NdotH / (4.f * HdotV) + 0.0001f; 

            float saTexel  = 4.f * PI / (6.f * u_f_source_resolution * u_f_source_resolution);
            float saSample = 1.f / (float(SAMPLE_COUNT) * pdf + 0.0001f);

            float mipLevel = u_f_roughness == 0.f ? 0.f : 0.5f * log2(saSample / saTexel);
			prefilteredColor += textureLod(SAMPLER_CUBE_0, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

	out_color = vec4(prefilteredColor, 1.f);
}