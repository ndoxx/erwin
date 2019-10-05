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

#type geometry
#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

layout(location = 2) in vec2 v_uv[];
layout(location = 3) out vec2 f_uv;

uniform mat4 u_view_projection;

void main()
{
    for(int ii=0; ii<gl_in.length(); ii++)
    {
    	gl_Position = u_view_projection*gl_in[ii].gl_Position;
    	f_uv = v_uv[ii];

   		EmitVertex();
    }
    EndPrimitive();

    gl_Position = u_view_projection*gl_in[2].gl_Position;
    f_uv = v_uv[2];
   	EmitVertex();

    gl_Position = u_view_projection*vec4(gl_in[0].gl_Position.x, gl_in[2].gl_Position.y, 0.f, 1.f);
    f_uv = vec2(v_uv[0].x, v_uv[2].y);
    // Be sure to set UVs when we get there
   	EmitVertex();

    gl_Position = u_view_projection*gl_in[0].gl_Position;
    f_uv = v_uv[0];
   	EmitVertex();
    EndPrimitive();
}

#type fragment
#version 460 core

layout(binding = 0) uniform sampler2D us_atlas;
layout(location = 3) in vec2 f_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(us_atlas, f_uv);
}
