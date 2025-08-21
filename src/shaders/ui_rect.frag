#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec4 modulate;
    float time;
};

void main() {
    FragColor = v_color * modulate;
}
