#include "sprite_renderer.hpp"

#include <SDL3/SDL_log.h>
#include <cmath>

// #include "component_storage.hpp"
#include "entity_manager.hpp"
#include "image.hpp"
#include "image_loader.hpp"
#include "sprite_component.hpp"
#include "transform_component.hpp"

namespace SpriteRenderer {
void upload_sprites(
    Renderer &renderer,
    std::unordered_map<EntityID, SpriteComponent> &sprite_components) {
  std::vector<std::string> loaded_sprite_paths;
  for (auto &[entity_id, sprite_component] : sprite_components) {
    if (std::find(loaded_sprite_paths.begin(), loaded_sprite_paths.end(),
                  sprite_component.path) != loaded_sprite_paths.end()) {
      // Sprite already loaded
      continue;
    }

    Image image = ImageLoader::load(sprite_component.path);
    TextureID texture_id =
        renderer.load_texture(image.pixels.data(), image.width, image.height);

    Texture texture_data;
    texture_data.path = sprite_component.path;
    texture_data.tiling = false;
    texture_data.id = texture_id;

    SDL_Log("Loaded texture %s with id %zu", sprite_component.path.c_str(),
            texture_id);

    sprite_component.size = glm::ivec2(image.width, image.height);
    sprite_component.texture_id = texture_id;
    loaded_sprite_paths.push_back(sprite_component.path);
  }
}

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
