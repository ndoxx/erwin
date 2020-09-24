#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;

void main()
{
    gl_Position = vec4(a_position, 1.f);
}


#type geometry
#version 460 core

layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 4) out;
out int gl_Layer;

void main() 
{     
    const vec2 vert_data[4] = vec2[]( vec2(-1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0) );

    for(int ii=0; ii<4; ++ii)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = vec4(vert_data[ii].xy,0,1);
        EmitVertex();
    }

    EndPrimitive();
}


#type fragment
#version 460 core

layout(location = 0) out vec4 out_color;
in int gl_Layer;

void main()
{
	float intensity = float(gl_Layer+1)/6.f;
    out_color = vec4(intensity,0.5f,1.f-intensity,1.f);
}