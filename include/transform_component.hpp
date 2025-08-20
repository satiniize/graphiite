#pragma once

#include "glm/glm.hpp"

struct TransformComponent {
  glm::vec2 position = glm::vec2(0.0f, 0.0f);
  float rotation = 0.0f; // Radians
  glm::vec2 scale = glm::vec2(1.0f, 1.0f);
};
