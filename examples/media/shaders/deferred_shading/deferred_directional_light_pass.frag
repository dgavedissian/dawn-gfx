#version 450 core

layout(location = 0) in VertexData {
    vec2 texcoord;
} i;

layout(location = 0) out vec4 out_colour;

/*
GBUFFER LAYOUT:
gb0: |diffuse.rgb|X|
gb1: |position.xyz|X|
gb2: |normal.xyz|X|
*/
layout(binding = 1) uniform sampler2D gb0_texture;
layout(binding = 2) uniform sampler2D gb1_texture;
layout(binding = 3) uniform sampler2D gb2_texture;

layout(binding = 4) uniform PerSubmit {
    vec3 light_direction;
} u;

void main() {
    vec4 gb0 = texture(gb0_texture, i.texcoord);
    vec4 gb1 = texture(gb1_texture, i.texcoord);
    vec4 gb2 = texture(gb2_texture, i.texcoord);

    // Extract attributes.
    vec3 diffuse = gb0.rgb;
    vec3 position = gb1.xyz;
    vec3 normal = gb2.xyz;

    // Render directional light.
    out_colour = vec4(clamp(dot(normal, u.light_direction), 0.0, 1.0) * diffuse, 1.0);
}
