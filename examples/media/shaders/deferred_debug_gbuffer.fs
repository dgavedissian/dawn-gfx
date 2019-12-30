#version 330 core

in VertexData {
    vec2 texcoord;
} i;

layout(location = 0) out vec4 out_colour;

uniform sampler2D gb0_texture;
uniform sampler2D gb1_texture;
uniform sampler2D gb2_texture;

void main()
{
    vec4 gb0 = texture(gb0_texture, i.texcoord);
    vec4 gb1 = texture(gb1_texture, i.texcoord);
    vec4 gb2 = texture(gb2_texture, i.texcoord);

    if (i.texcoord.x < 0.5 && i.texcoord.y >= 0.5) {
        out_colour = texture(gb0_sampler, (i.texcoord - vec2(0.0, 0.5)) * 2.0);
    } else if (i.texcoord.x >= 0.5 && i.texcoord.y >= 0.5) {
        out_colour = texture(gb1_sampler, (i.texcoord - vec2(0.5, 0.5)) * 2.0);
    } else if (i.texcoord.x < 0.5 && i.texcoord.y < 0.5) {
        out_colour = texture(gb2_sampler, (i.texcoord - vec2(0.0, 0.0)) * 2.0);
    } else {
        out_colour = vec4(0, 0, 0, 0);
    }
}
