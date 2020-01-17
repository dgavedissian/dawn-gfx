#version 420 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in vec3 in_position;

layout(binding = 0) uniform Matrices {
    mat4 mvp_matrix;
} u;

void main()
{
    gl_Position = u.mvp_matrix * vec4(in_position, 1.0);
}