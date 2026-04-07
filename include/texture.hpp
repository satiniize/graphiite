#pragma once
#include "image.hpp"
#include <cstdint>
#include <string>

// Texture is just a handle class, this does not own the texture data

using TextureID = std::size_t;

class Texture {
public:
  std::string path;
  bool tiling; // TODO: Change this to fill mode? idk
  TextureID id;

  // Derived from Image for downloading
  uint8_t channels;
  uint16_t width;
  uint16_t height;
  PixelFormat pixel_format = PixelFormat::RGBA8;

  Texture() = default;
  uint8_t bytes_per_pixel() const { return static_cast<uint8_t>(pixel_format); }
};
