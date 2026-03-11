#pragma once
#include <string>

// Texture is just a handle class, this does not own the texture data

using TextureID = std::size_t;

struct Texture {
  std::string path;
  bool tiling;
  TextureID id;
};
