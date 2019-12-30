#version 330 core

in VertexData {
    vec2 texcoord;
    mat3 tbn;
} i;

layout(location = 0) out vec4 out_colour;

uniform vec3 light_direction;

uniform sampler2D diffuse_texture;
uniform sampler2D normal_map_texture;

void main()
{
    // Compute normal.
    vec3 normal = texture(normal_map_texture, i.texcoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(i.tbn * normal);

    // Diffuse lighting.
    vec4 diffuse = texture(diffuse_texture, i.texcoord);
    out_colour = clamp(dot(normal, light_direction), 0.0, 1.0) * diffuse;
}