#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2D myTextureSampler;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    float time;
};

void main() {
    // float pulse = sin(time * 2.0) * 0.5 + 0.5; // range [0, 1]
    vec4 albedo = texture(myTextureSampler, v_texcoord);
    // FragColor = vec4(albedo.rgb * v_color.rgb * pulse, albedo.a * v_color.a);
    FragColor = vec4(albedo.rgb * v_color.rgb, albedo.a * v_color.a);
}
