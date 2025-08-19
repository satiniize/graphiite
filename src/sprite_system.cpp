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
void draw_all() {
  for (auto &[entity, sprite] : sprite_components) {
    auto it = transform_components.find(entity);
    if (it != transform_components.end()) {
      calculate_aabb(it->second, sprite, sprite.rendering_aabb.top_left,
                     sprite.rendering_aabb.bottom_right);
    }
  }
}

} // namespace SpriteSystem
