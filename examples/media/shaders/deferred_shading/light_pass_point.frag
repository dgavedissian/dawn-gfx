#version 450 core

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
    vec2 screen_size;

    vec3 light_position;
    vec3 light_colour;
    float linear_term;
    float quadratic_term;
};

void main() {
    vec2 screen_coord = gl_FragCoord.xy / screen_size;

    vec3 diffuse_colour = texture(gb0_texture, screen_coord).rgb;
    vec3 pixel_position = texture(gb1_texture, screen_coord).rgb;
    vec3 pixel_normal = normalize(texture(gb2_texture, screen_coord).rgb);

    vec3 lighting = vec3(0.0);

    // Diffuse.
    vec3 light_dir = normalize(light_position - pixel_position);
    vec3 diffuse = max(dot(pixel_normal, light_dir), 0.0) * diffuse_colour * light_colour;
    // Attenuation.
    float distance = length(light_position - pixel_position);
    float attenuation = 1.0 / (1.0 + linear_term * distance + quadratic_term * distance * distance);
    diffuse *= attenuation;
    lighting += diffuse;

    out_colour = vec4(lighting, 1.0);
}