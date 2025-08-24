#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec4 modulate;
    float relative_thickness;
};

void main() {
    float alpha = 1.0f;
    float dist = length(v_texcoord - vec2(0.0f, 1.0f));
    if (dist > 1.0f || dist < 1.0f - relative_thickness) {
        alpha = 0.0f;
    }
    vec4 color = v_color * modulate;
    FragColor = vec4(color.rgb, alpha);
}
