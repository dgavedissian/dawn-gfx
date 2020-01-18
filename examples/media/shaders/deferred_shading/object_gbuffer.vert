#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(binding = 0) uniform PerSubmit {
    mat4 model_matrix;
    mat4 mvp_matrix;
} u;

layout(location = 0) out VertexData {
    vec3 world_position;
    vec3 normal;
    vec2 texcoord;
} o;

void main() {
    gl_Position = u.mvp_matrix * vec4(in_position, 1.0);
    o.world_position = (u.model_matrix * vec4(in_position, 1.0)).xyz;
    o.normal = (u.model_matrix * vec4(in_normal, 0.0)).xyz;
    o.texcoord = in_texcoord;
}