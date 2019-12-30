#version 330 core

layout (location = 0) in vec3 in_position;

uniform mat4 mvp_matrix;

void main()
{
    gl_Position = mvp_matrix * vec4(in_position, 1.0);
}