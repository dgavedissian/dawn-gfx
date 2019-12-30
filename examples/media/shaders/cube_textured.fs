#version 330 core

in VertexData {
    vec2 texcoord;
    vec3 normal;
} i;

layout(location = 0) out vec4 out_colour;

uniform vec3 light_direction;
uniform sampler2D diffuse_texture;

void main()
{
    vec4 diffuse = texture(diffuse_texture, i.texcoord);
    out_colour = clamp(dot(i.normal, light_direction), 0.0, 1.0) * diffuse;
}