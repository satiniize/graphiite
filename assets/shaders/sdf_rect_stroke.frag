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
    vec4 inner_corner_radii;
    vec4 stroke_thickness;
    int tiling;
    int use_texture;
    int draw_stroke;
    float smoothing;
};

// TODO: Radii here is multiplied by 2 for compensation
// TODO: Somehow interpolate radii from 2x to 1x and interpolate power from 4 to 2 on pill shapes
float rounded_rect_sdf_per_corner(vec2 pos, vec2 half_extents, vec4 radii) {
    // radii: x=top-left, y=top-right, z=bottom-left, w=bottom-right
    float power = 4;
    vec4 scaled_radii = radii * 2.0;
    float r = pos.x < 0.0
        ? (pos.y < 0.0 ? scaled_radii.x : scaled_radii.z) : (pos.y < 0.0 ? scaled_radii.y : scaled_radii.w);
    vec2 d = abs(pos) - half_extents + r;
    vec2 d_clamped = max(d, 0.0);
    float outer = pow(pow(d_clamped.x, power) + pow(d_clamped.y, power), 1.0 / power);
    return outer + min(max(d.x, d.y), 0.0) - r;
}

void main() {
    vec2 padded = size.zw; // rendered quad size
    vec2 rect_size = size.xy; // logical rect size

    vec2 pos = v_texcoord * padded;
    vec2 half_extents = rect_size / 2.0;
    vec2 padded_half = padded / 2.0;
    // Left right top bottom | xyzw
    vec2 inner_rect_offset = vec2(stroke_thickness.x - stroke_thickness.y, stroke_thickness.z - stroke_thickness.w) / 2.0;
    vec2 inner_rect_shrink = vec2(stroke_thickness.x + stroke_thickness.y, stroke_thickness.z + stroke_thickness.w) / 2.0;
    float outer_dist = rounded_rect_sdf_per_corner(pos - padded_half, half_extents, corner_radii);
    float inner_dist = rounded_rect_sdf_per_corner(pos - padded_half - inner_rect_offset, half_extents - inner_rect_shrink, inner_corner_radii);

    // float smoothing = 1.0;
    float base = draw_stroke == 1 ? smoothstep(-smoothing * 0.5, smoothing * 0.5, inner_dist) : 1.0;
    float alpha = base - smoothstep(-smoothing * 0.5, smoothing * 0.5, outer_dist);

    vec2 sample_uv = tiling == 1 ? v_texcoord * size.xy / 16.0 : v_texcoord;
    vec4 albedo = texture(myTextureSampler, sample_uv);
    vec4 color = v_color * modulate;
    color *= mix(vec4(1.0), albedo, float(use_texture));

    float final_alpha = color.a * alpha;
    FragColor = vec4(color.rgb * final_alpha, final_alpha);
}
