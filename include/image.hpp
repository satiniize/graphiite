#pragma once

#include <cstdint>
#include <functional>
// #include <glm/glm.hpp>
#include <clay.h>
#include <vector>

enum class PixelFormat : uint8_t {
  RGBA8 = 4,  // R8G8B8A8_UNORM
  RGBA16 = 8, // R16G16B16A16_UNORM
};

struct GradientStop {
  float position; // 0.0 to 1.0
  Clay_Color color;
};

using ImageGenerator =
    std::function<Clay_Color(float x, float y, float w, float h)>;

class Image {
public:
  std::vector<uint8_t> pixels;
  uint8_t channels;
  uint16_t width;
  uint16_t height;
  PixelFormat pixel_format = PixelFormat::RGBA8;

  Image() = default;
  Image(uint16_t width, uint16_t height,
        PixelFormat pixel_format = PixelFormat::RGBA8);

  static ImageGenerator angular_gradient(std::vector<GradientStop> stops);
  static ImageGenerator linear_gradient(float angle,
                                        std::vector<GradientStop> stops);
  static ImageGenerator checkerboard(Clay_Color color1, Clay_Color color2);
  static ImageGenerator noise();

  uint8_t bytes_per_pixel() const { return static_cast<uint8_t>(pixel_format); }
  void fill(ImageGenerator generator);

  void clear();
};
