#version 450 core

layout(location = 0) in vec3 in_position;

layout(binding = 0) uniform PerSubmit {
    mat4 mvp_matrix;
} u;

void main() {
    gl_Position = u.mvp_matrix * vec4(in_position, 1.0);
}