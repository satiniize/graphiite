#pragma once

#include "vec2.hpp"

// Pure ECS data-only component for transform (position, rotation, scale)
struct TransformComponent {
  vec2 position = vec2(0.0f, 0.0f);
  float rotation = 0.0f; // Radians
  vec2 scale = vec2(1.0f, 1.0f);
};
