#pragma once

#include <string>

#include "glm/vec2.hpp"

struct SpriteComponent {
  std::string path;
  glm::ivec2 size;
};
