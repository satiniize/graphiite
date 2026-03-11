#include "sprite_renderer.hpp"

#include <cmath>

// #include "component_storage.hpp"
#include "entity_manager.hpp"
#include "sprite_component.hpp"
#include "transform_component.hpp"

namespace SpriteRenderer {

void draw_all(
    Renderer &renderer,
    std::unordered_map<EntityID, SpriteComponent> &sprite_components,
    std::unordered_map<EntityID, TransformComponent> &transform_components) {
  for (auto &[entity, sprite] : sprite_components) {
    auto it = transform_components.find(entity);
    if (it != transform_components.end()) {
      TransformComponent transform = it->second;
      renderer.draw_sprite(sprite.texture_id, transform.position,
                           transform.rotation,
                           transform.scale * glm::vec2(sprite.size),
                           glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
  }
}

} // namespace SpriteRenderer
