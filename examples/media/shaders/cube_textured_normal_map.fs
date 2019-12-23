#version 330 core

in vec2 TexCoord;
in mat3 TBN;
in vec3 Color;

layout(location = 0) out vec4 outColor;

uniform vec3 light_dir;

uniform sampler2D wall_sampler;
uniform sampler2D normal_sampler;

void main()
{
    // Compute normal.
    vec3 normal = texture(normal_sampler, TexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(TBN * normal);

    // Diffuse lighting.
    vec4 diffuse = texture(wall_sampler, TexCoord);
    outColor = clamp(dot(normal, light_dir), 0.0, 1.0) * vec4(Color, 1.0) * diffuse;
}