#version 330 core

in VertexData {
    vec2 texcoord;
} i;

layout(location = 0) out vec4 out_colour;

/*
GBUFFER LAYOUT:
gb0: |diffuse.rgb|X|
gb1: |position.xyz|X|
gb2: |normal.xyz|X|
*/
uniform sampler2D gb0_texture;
uniform sampler2D gb1_texture;
uniform sampler2D gb2_texture;

uniform vec3 ambient_light;

void main()
{
    vec4 gb0 = texture(gb0_texture, i.texcoord);
    vec4 gb1 = texture(gb1_texture, i.texcoord);
    vec4 gb2 = texture(gb2_texture, i.texcoord);

    // Extract attributes.
    vec3 diffuse = gb0.rgb;
    vec3 position = gb1.xyz;
    vec3 normal = gb2.xyz;

    // Render directional light.
    if (diffuse.rgb == vec3(0.0, 0.0, 0.0)) {
        out_colour = vec4(diffuse, 1.0);
    } else {
        out_colour = vec4(diffuse * ambient_light, 1.0);
    }
}
