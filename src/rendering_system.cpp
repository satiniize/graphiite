#include "component_storage.hpp"
#include "sprite_component.hpp"
#include "transform_component.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace SpriteSystem {

// Utility: Calculate AABB for a transformed rectangle centered at position
void calculate_aabb(const TransformComponent &transform,
                    const SpriteComponent &sprite, vec2 &out_top_left,
                    vec2 &out_bottom_right) {
  float x_ext = sprite.size.x / 2.0f;
  float y_ext = sprite.size.y / 2.0f;

  // Four corners in local space
  vec2 corners[4] = {vec2(-x_ext, -y_ext), vec2(x_ext, -y_ext),
                     vec2(-x_ext, y_ext), vec2(x_ext, y_ext)};

  // Apply rotation and translation
  float cos_r = std::cos(transform.rotation);
  float sin_r = std::sin(transform.rotation);

  std::vector<vec2> transformed;
  for (int i = 0; i < 4; ++i) {
    float x = corners[i].x * transform.scale.x;
    float y = corners[i].y * transform.scale.y;
    // Rotate
    float rx = x * cos_r - y * sin_r;
    float ry = x * sin_r + y * cos_r;
    // Translate
    transformed.push_back(
        vec2(rx + transform.position.x, ry + transform.position.y));
  }

  float min_x = transformed[0].x, max_x = transformed[0].x;
  float min_y = transformed[0].y, max_y = transformed[0].y;
  for (const auto &v : transformed) {
    min_x = std::min(min_x, v.x);
    max_x = std::max(max_x, v.x);
    min_y = std::min(min_y, v.y);
    max_y = std::max(max_y, v.y);
  }
  out_top_left = vec2(min_x, min_y);
  out_bottom_right = vec2(max_x, max_y);
}

// System: Update AABBs for all sprites
void update_aabbs() {
  for (auto &[entity, sprite] : sprite_components) {
    auto it = transform_components.find(entity);
    if (it != transform_components.end()) {
      calculate_aabb(it->second, sprite, sprite.rendering_aabb.top_left,
                     sprite.rendering_aabb.bottom_right);
    }
  }
}

// System: Draw all sprites with transforms to framebuffer
void draw_all(uint32_t *framebuffer, int framebuffer_width,
              int framebuffer_height) {
  for (const auto &[entity, sprite] : sprite_components) {
    auto it = transform_components.find(entity);
    if (it == transform_components.end())
      continue;
    const TransformComponent &transform = it->second;

    // For each pixel in the sprite's bitmap
    int bmp_width = sprite.bitmap.get_width();
    int bmp_height = sprite.bitmap.get_height();
    const std::vector<uint32_t> &pixels = sprite.bitmap.get_pixels();

    // Center of sprite in local space
    float cx = bmp_width / 2.0f;
    float cy = bmp_height / 2.0f;

    // Inverse transform for mapping framebuffer pixel to sprite pixel
    float cos_r = std::cos(-transform.rotation);
    float sin_r = std::sin(-transform.rotation);

    // Only draw within the AABB
    int min_x = std::max(
        0, static_cast<int>(std::floor(sprite.rendering_aabb.top_left.x)));
    int max_x = std::min(
        framebuffer_width,
        static_cast<int>(std::ceil(sprite.rendering_aabb.bottom_right.x)));
    int min_y = std::max(
        0, static_cast<int>(std::floor(sprite.rendering_aabb.top_left.y)));
    int max_y = std::min(
        framebuffer_height,
        static_cast<int>(std::ceil(sprite.rendering_aabb.bottom_right.y)));

    for (int y = min_y; y < max_y; ++y) {
      for (int x = min_x; x < max_x; ++x) {
        // Transform framebuffer pixel (x, y) to sprite local space
        float fx = x - transform.position.x;
        float fy = y - transform.position.y;
        // Inverse rotate
        float lx = fx * cos_r - fy * sin_r;
        float ly = fx * sin_r + fy * cos_r;
        // Inverse scale and shift to bitmap coordinates
        float sx = lx / transform.scale.x + cx;
        float sy = ly / transform.scale.y + cy;

        int ix = static_cast<int>(std::floor(sx));
        int iy = static_cast<int>(std::floor(sy));
        if (ix >= 0 && ix < bmp_width && iy >= 0 && iy < bmp_height) {
          uint32_t pixel = pixels[iy * bmp_width + ix];
          if ((pixel & 0xFF) != 0) { // Gets alpha from RGBA
            framebuffer[y * framebuffer_width + x] = pixel;
          }
        }
      }
    }
  }
}

} // namespace SpriteSystem
