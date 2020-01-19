#version 450 core

layout(location = 0) in VertexData {
    vec3 world_position;
    vec3 normal;
    vec2 texcoord;
} i;

/*
GBUFFER LAYOUT:
gb0: |diffuse.rgb|X|
gb1: |position.xyz|X|
gb2: |normal.xyz|X|
*/
layout(location = 0) out vec4 gb0;
layout(location = 1) out vec4 gb1;
layout(location = 2) out vec4 gb2;

layout(binding = 1) uniform PerSubmit {
    vec2 texcoord_scale;
};

layout(binding = 2) uniform sampler2D diffuse_texture;

void main() {
    vec4 diffuse = texture(diffuse_texture, i.texcoord * texcoord_scale);
    gb0 = vec4(diffuse.rgb, 0.0);
    gb1 = vec4(i.world_position, 0.0);
    gb2 = vec4(normalize(i.normal), 0.0);
}