#pragma once

#include "entity_manager.hpp"
#include "renderer.hpp"
#include "sprite_component.hpp"
#include "transform_component.hpp"

namespace SpriteRenderer {
void upload_sprites(
    Renderer &renderer,
    std::unordered_map<EntityID, SpriteComponent> &sprite_components);
void draw_all(
    Renderer &renderer,
    std::unordered_map<EntityID, SpriteComponent> &sprite_components,
    std::unordered_map<EntityID, TransformComponent> &transform_components);
} // namespace SpriteRenderer
