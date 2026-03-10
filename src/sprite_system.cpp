#include "sprite_system.hpp"

#include <cmath>

#include "component_storage.hpp"
#include "transform_component.hpp"

namespace SpriteSystem {

void draw_all(Renderer &renderer) {
  for (auto &[entity, sprite] : sprite_components) {
    auto it = transform_components.find(entity);
    if (it != transform_components.end()) {
      TransformComponent transform = it->second;
      // renderer.draw_sprite(sprite.path, transform.position,
      // transform.rotation,
      //                      transform.scale * glm::vec2(sprite.size),
      //                      glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
  }
}

} // namespace SpriteSystem
