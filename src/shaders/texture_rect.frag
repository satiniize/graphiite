#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2D myTextureSampler;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec4 size;
    vec4 modulate;
    vec4 corner_radii;
    int tiling;
};

void main() {
    vec2 pos = vec2(v_texcoord.x * size.x, v_texcoord.y * size.y);
    float smoothing = 1.0f;
    float max_radius = min(size.x, size.y) / 2.0f;

    float alpha = 1.0f;
    float radius;
    float dist;
    // TODO: Have proper radius clamping
    radius = min(corner_radii.x, max_radius);
    dist = length(pos - vec2(radius, radius));
    if (pos.x < radius && pos.y < radius) {
        alpha = 1.0f - smoothstep(radius - smoothing / 2.0f, radius + smoothing / 2.0f, dist);
    }
    radius = min(corner_radii.y, max_radius);
    dist = length(pos - vec2(size.x - radius, radius));
    if (pos.x > size.x - radius && pos.y < radius) {
        alpha = 1.0f - smoothstep(radius - smoothing / 2.0f, radius + smoothing / 2.0f, dist);
    }
    radius = min(corner_radii.z, max_radius);
    dist = length(pos - vec2(radius, size.y - radius));
    if (pos.x < radius && pos.y > size.y - radius) {
        alpha = 1.0f - smoothstep(radius - smoothing / 2.0f, radius + smoothing / 2.0f, dist);
    }
    radius = min(corner_radii.w, max_radius);
    dist = length(pos - vec2(size.x - radius, size.y - radius));
    if (pos.x > size.x - radius && pos.y > size.y - radius) {
        alpha = 1.0f - smoothstep(radius - smoothing / 2.0f, radius + smoothing / 2.0f, dist);
    }
    vec4 color = v_color * modulate;
    vec2 sample_uv;
    if (tiling == 1) {
        sample_uv = v_texcoord * size.xy / 16.0f;
    } else {
        sample_uv = v_texcoord;
    }
    vec4 albedo = texture(myTextureSampler, sample_uv);
    FragColor = vec4(albedo.rgb * v_color.rgb * modulate.rgb, albedo.a * v_color.a * modulate.a * alpha);
}
