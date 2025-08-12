#pragma once

#include "vec2.hpp"

class AABB {
public:
  AABB() = default;
  AABB(const vec2 &top_left, const vec2 &bottom_right)
      : top_left(top_left), bottom_right(bottom_right) {}

  AABB to_world_space(vec2 position) const {
    return AABB(position + top_left, position + bottom_right);
  }

  vec2 top_left;
  vec2 bottom_right;
};
