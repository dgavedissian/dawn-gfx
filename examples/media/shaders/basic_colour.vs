#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_enhanced_layouts : enable

//layout(location = 0) in vec2 in_position;
//layout(location = 1) in vec3 in_colour;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(location = 0) out VertexData {
    vec3 colour;
} o;

void main()
{
    o.colour = colors[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    //o.colour = in_colour;
    //gl_Position = vec4(in_position, 0.0, 1.0);
}
