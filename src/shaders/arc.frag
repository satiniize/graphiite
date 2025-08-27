#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;
// TODO: Rename this bullshit to arc
// TODO: Add graduated thickness
layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec4 modulate;
    float radius;
    float thickness;
    float padding1;
    float padding2;
};

void main() {
    vec2 pos = v_texcoord * radius;
    float smoothing = 1.0f;
    float alpha = 1.0f;
    float dist = length(pos - vec2(0.0f, radius));
    if (dist > radius - smoothing / 2.0f) {
        float start = radius - smoothing / 2.0f;
        float end = radius + smoothing / 2.0f;
        alpha = 1.0f - smoothstep(start, end, dist);
    }
    if (dist < radius - thickness + smoothing / 2.0f) {
        float start = radius - thickness - smoothing / 2.0f;
        float end = radius - thickness + smoothing / 2.0f;
        alpha = smoothstep(start, end, dist);
    }
    vec4 color = v_color * modulate;
    FragColor = vec4(color.rgb, alpha);
}
