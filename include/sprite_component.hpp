#pragma once

#include "aabb.hpp"
#include "bitmap.hpp"
#include "vec2.hpp"
#include <SDL3/SDL.h>
#include <string>

// Pure ECS data-only component for sprite rendering
struct SpriteComponent {
  // Bitmap bitmap;
  std::string path;
  vec2 size;
  AABB rendering_aabb;
};
