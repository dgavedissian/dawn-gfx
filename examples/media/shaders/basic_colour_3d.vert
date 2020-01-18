#version 450 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_colour;

layout(location = 0) out VertexData {
    vec3 colour;
} o;

layout(binding = 0) uniform PerSubmit {
    mat4 mvp_matrix;
} u;

void main() {
    o.colour = in_colour;
    gl_Position = u.mvp_matrix * vec4(in_position, 0.0, 1.0);
}
