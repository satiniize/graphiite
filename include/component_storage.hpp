#pragma once

#include "entity_manager.hpp"
#include "rigidbody_component.hpp"
#include "sprite_component.hpp"
#include "staticbody_component.hpp"
#include "transform_component.hpp"
#include <unordered_map>

// ECS component storage for each component type.
// Each map stores components by EntityID.

inline std::unordered_map<EntityID, SpriteComponent> sprite_components;
inline std::unordered_map<EntityID, TransformComponent> transform_components;
inline std::unordered_map<EntityID, RigidBodyComponent> rigidbody_components;
inline std::unordered_map<EntityID, StaticBodyComponent> staticbody_components;
