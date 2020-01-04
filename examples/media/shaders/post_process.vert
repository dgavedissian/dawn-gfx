#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

out VertexData {
    vec2 texcoord;
} o;

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    o.texcoord = in_texcoord;
}