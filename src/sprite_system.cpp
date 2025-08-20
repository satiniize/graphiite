#include "sprite_system.hpp"
#include "component_storage.hpp"
#include "sprite_component.hpp"
#include "transform_component.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace SpriteSystem {

void draw_all(Renderer &renderer) {
  for (auto &[entity, sprite] : sprite_components) {
    auto it = transform_components.find(entity);
    if (it != transform_components.end()) {
      TransformComponent transform = it->second;
      renderer.draw_sprite(sprite.path, transform.position, transform.rotation,
                           transform.scale);
    }
  }
}

} // namespace SpriteSystem
