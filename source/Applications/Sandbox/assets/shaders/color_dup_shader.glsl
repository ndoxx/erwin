#type vertex
#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

out vec3 v_color;

void main()
{
	gl_Position = vec4(in_position, 1.f);
	v_color = in_color;
}

#type geometry
#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in vec3 v_color[];
out vec3 f_color;

void main()
{
    for(int ii=0; ii<gl_in.length(); ii++)
    {
    	gl_Position = gl_in[ii].gl_Position;
    	f_color = v_color[ii];

   		EmitVertex();
    }
    EndPrimitive();

    gl_Position = gl_in[2].gl_Position;
    f_color = v_color[2];
   	EmitVertex();

    gl_Position = vec4(gl_in[0].gl_Position.x, gl_in[2].gl_Position.y, 0.f, 1.f);
    f_color = v_color[1];
    // Be sure to set UVs when we get there
   	EmitVertex();

    gl_Position = gl_in[0].gl_Position;
    f_color = v_color[0];
   	EmitVertex();
    EndPrimitive();
}

#type fragment
#version 460 core

in vec3 f_color;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(f_color,1.0f);
}
