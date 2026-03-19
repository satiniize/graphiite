#pragma once

#include <cstdint>
#include <vector>

enum class PixelFormat : uint8_t {
  RGBA8 = 4,  // R8G8B8A8_UNORM
  RGBA16 = 8, // R16G16B16A16_UNORM
};

struct Image {
  std::vector<uint8_t> pixels;
  uint8_t channels;
  uint16_t width;
  uint16_t height;
  PixelFormat format;

  uint8_t bytes_per_pixel() const { return static_cast<uint8_t>(format); }
};
