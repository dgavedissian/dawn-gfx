#version 330 core

in VertexData {
    vec3 normal;
} i;

layout(location = 0) out vec4 out_colour;

uniform vec3 light_direction;
uniform vec3 diffuse_colour = vec3(1.0, 1.0, 1.0);

void main()
{
    out_colour = clamp(dot(i.normal, light_direction), 0.0, 1.0) * vec4(diffuse_colour, 1.0);
}