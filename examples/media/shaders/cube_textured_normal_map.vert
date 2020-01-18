#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec3 in_tangent;

layout(binding = 0) uniform PerSubmit {
    mat4 model_matrix;
    mat4 mvp_matrix;
} u;

layout(location = 0) out VertexData {
    vec2 texcoord;
    mat3 tbn;
} o;

void main() {
    vec3 t = normalize((u.model_matrix * vec4(in_tangent, 0.0)).xyz);
    vec3 n = normalize((u.model_matrix * vec4(in_normal, 0.0)).xyz);
    vec3 b = cross(n, t);
    o.tbn = mat3(t, b, n);

    o.texcoord = in_texcoord;
    gl_Position = u.mvp_matrix * vec4(in_position, 1.0);
}