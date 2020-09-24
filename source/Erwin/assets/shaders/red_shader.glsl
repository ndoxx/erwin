#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;

void main()
{
    gl_Position = vec4(a_position, 1.f);
}

#type fragment
#version 460 core

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(1.f,0.f,0.f,1.f);
}