#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec3 in_tangent;

uniform mat4 model_matrix;
uniform mat4 mvp_matrix;

out VertexData {
    vec2 texcoord;
    mat3 tbn;
} o;

void main()
{
    vec3 t = normalize((model_matrix * vec4(in_tangent, 0.0)).xyz);
    vec3 n = normalize((model_matrix * vec4(in_normal, 0.0)).xyz);
    vec3 b = cross(n, t);
    o.tbn = mat3(t, b, n);

    o.texcoord = in_texcoord;
    gl_Position = mvp_matrix * vec4(in_position, 1.0);
}