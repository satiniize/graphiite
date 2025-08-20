#pragma once

#include <unordered_map>

#include "entity_manager.hpp"
#include "sprite_component.hpp"
#include "transform_component.hpp"

inline std::unordered_map<EntityID, SpriteComponent> sprite_components;
inline std::unordered_map<EntityID, TransformComponent> transform_components;
