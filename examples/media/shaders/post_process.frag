#version 450 core

layout(location = 0) in VertexData {
    vec2 texcoord;
} i;

layout(location = 0) out vec4 out_colour;

layout(binding = 1) uniform sampler2D input_texture;

void main() {
    vec4 fb_input = texture(input_texture, i.texcoord);
    out_colour = vec4(vec3(1.0) - fb_input.rgb, 1.0);
}
