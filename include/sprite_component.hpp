#pragma once

#include "aabb.hpp"
#include "bitmap.hpp"
#include "vec2.hpp"
#include <SDL3/SDL.h>

// Pure ECS data-only component for sprite rendering
struct SpriteComponent {
  Bitmap bitmap;
  vec2 size;
  AABB rendering_aabb;
};
