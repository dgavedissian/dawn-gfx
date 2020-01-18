#version 450 core

layout(location = 0) in VertexData {
    vec2 texcoord;
    mat3 tbn;
} i;

layout(location = 0) out vec4 out_colour;

layout(binding = 1) uniform PerSubmit {
    vec3 light_direction;
} u;

layout(binding = 2) uniform sampler2D diffuse_texture;
layout(binding = 3) uniform sampler2D normal_map_texture;

void main() {
    // Compute normal.
    vec3 normal = texture(normal_map_texture, i.texcoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(i.tbn * normal);

    // Diffuse lighting.
    vec4 diffuse = texture(diffuse_texture, i.texcoord);
    out_colour = clamp(dot(normal, u.light_direction), 0.0, 1.0) * diffuse;
}