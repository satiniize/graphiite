#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2D myTextureSampler;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec4 modulate;
    vec4 uv_rect;
};

void main() {
    vec2 sampled_uv = uv_rect.xy + v_texcoord * (uv_rect.zw - uv_rect.xy);
    vec4 albedo = texture(myTextureSampler, sampled_uv);
    FragColor = vec4(albedo.rgb * v_color.rgb * modulate.rgb, albedo.a * v_color.a * modulate.a);
}
