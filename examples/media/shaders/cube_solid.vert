#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

uniform mat4 model_matrix;
uniform mat4 mvp_matrix;

out VertexData {
    vec3 normal;
} o;

void main()
{
    o.normal = (model_matrix * vec4(in_normal, 0.0)).xyz;
    gl_Position = mvp_matrix * vec4(in_position, 1.0);
}