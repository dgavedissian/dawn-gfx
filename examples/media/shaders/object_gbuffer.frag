#version 420 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_enhanced_layouts : enable

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

layout(binding = 1) uniform Parameters {
    vec2 texcoord_scale;
} u;

layout(binding = 2) uniform sampler2D diffuse_texture;

void main()
{
    vec4 diffuse = texture(diffuse_texture, i.texcoord * u.texcoord_scale);
    gb0.rgb = diffuse.rgb;
    gb1.rgb = i.world_position;
    gb2.rgb = normalize(i.normal);
}