#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec4 modulate;
    vec4 corner_radii;
    vec4 size;
};

void main() {
    float alpha = 1.0f;
    vec2 pos = vec2(v_texcoord.x * size.x, v_texcoord.y * size.y);
    float dist;
    float radius;
    // TODO: Have proper radius clamping
    radius = min(corner_radii.x, min(size.x, size.y));
    dist = length(pos - vec2(radius, radius));
    if (dist > radius && pos.x < radius && pos.y < radius) {
        alpha = 0.0f;
    }
    radius = min(corner_radii.y, min(size.x, size.y));
    dist = length(pos - vec2(size.x - radius, radius));
    if (dist > radius && pos.x > size.x - radius && pos.y < radius) {
        alpha = 0.0f;
    }
    radius = min(corner_radii.z, min(size.x, size.y));
    dist = length(pos - vec2(radius, size.y - radius));
    if (dist > radius && pos.x < radius && pos.y > size.y - radius) {
        alpha = 0.0f;
    }
    radius = min(corner_radii.w, min(size.x, size.y));
    dist = length(pos - vec2(size.x - radius, size.y - radius));
    if (dist > radius && pos.x > size.x - radius && pos.y > size.y - radius) {
        alpha = 0.0f;
    }
    vec4 color = v_color * modulate;
    FragColor = vec4(color.rgb, alpha);
}
