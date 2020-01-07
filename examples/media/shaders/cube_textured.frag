#version 420 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_enhanced_layouts : enable

layout(location = 0) in VertexData {
    vec2 texcoord;
    vec3 normal;
} i;

layout(location = 0) out vec4 out_colour;

layout(binding = 1) uniform LightInfo {
    vec3 light_direction;
} u;

layout(binding = 2) uniform sampler2D diffuse_texture;

void main()
{
    vec4 diffuse = texture(diffuse_texture, i.texcoord);
    out_colour = clamp(dot(i.normal, u.light_direction), 0.0, 1.0) * diffuse;
}