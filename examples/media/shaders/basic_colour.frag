#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in VertexData {
    vec3 colour;
} i;

layout(location = 0) out vec4 out_colour;

void main()
{
    out_colour = vec4(i.colour, 1.0);
}