#pragma once

#include <cstdint>
#include <vector>

// RGBA8888 format
struct Image {
  std::vector<uint8_t> pixels;
  uint8_t channels;
  uint16_t width;
  uint16_t height;
};
