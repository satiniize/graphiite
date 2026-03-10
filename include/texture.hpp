#pragma once
#include <string>

// Texture is just a handle class, this does not own the texture data
struct Texture {
  std::string path;
  bool tiling;
  size_t id;
};
