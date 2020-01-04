#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_colour;

layout(location = 0) out VertexData {
    vec3 colour;
} o;

void main()
{
    o.colour = in_colour;
    gl_Position = vec4(in_position, 0.0, 1.0);
}
