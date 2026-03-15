#version 460
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;
layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2D myTextureSampler;

layout(std140, set = 3, binding = 0) uniform CommonUniforms {
    float time;
};

layout(std140, set = 3, binding = 1) uniform UniformBlock {
    vec4 size;
    vec4 modulate;
    vec4 corner_radii;
    int tiling;
    int use_texture;
};

float rounded_rect_sdf_per_corner(vec2 pos, vec2 half_extents, vec4 radii) {
    // radii: x=top-left, y=top-right, z=bottom-left, w=bottom-right
    float r = pos.x < 0.0
        ? (pos.y < 0.0 ? radii.x : radii.z) : (pos.y < 0.0 ? radii.y : radii.w);
    vec2 d = abs(pos) - half_extents + r;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;
}

void main() {
    vec2 pos = vec2(v_texcoord.x * size.x, v_texcoord.y * size.y);
    vec2 half_extents = size.xy / 2.0f;

    float max_radius = min(half_extents.x, half_extents.y);
    vec4 clamped_radii = min(corner_radii, max_radius);

    float dist = rounded_rect_sdf_per_corner(pos, half_extents, clamped_radii);

    float smoothing = 1.0;
    float alpha = 1.0 - smoothstep(-smoothing * 0.5, smoothing * 0.5, dist);

    vec2 sample_uv = tiling == 1 ? v_texcoord * size.xy / 16.0 : v_texcoord;
    vec4 albedo = texture(myTextureSampler, sample_uv);
    vec4 color = v_color * modulate;
    color *= mix(vec4(1.0), albedo, float(use_texture));

    float final_alpha = color.a * alpha;
    FragColor = vec4(color.rgb * final_alpha, final_alpha);
}
