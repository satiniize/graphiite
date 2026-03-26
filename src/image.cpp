#include "image.hpp"

Image::Image(uint16_t width, uint16_t height, PixelFormat format) {
  int bytes = format == PixelFormat::RGBA8 ? 4 : 8;
  pixels.resize(width * height * bytes);
  this->format = format;
  this->width = width;
  this->height = height;
}

ImageGenerator Image::angular_gradient() {
  return [](float x, float y, float w, float h) -> glm::vec4 {
    float angle = atan2(y - h / 2.0f, x - w / 2.0f);
    float t = (angle + M_PI) / (2.0f * M_PI);
    return glm::vec4(t, t, t, 1.0f);
  };
}

void Image::fill(ImageGenerator generator) {
  int bytes = format == PixelFormat::RGBA8 ? 4 : 8;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      glm::vec4 color = generator(x, y, width, height);
      int base_index = (y * width + x) * bytes;
      pixels[base_index + 0] = static_cast<uint8_t>(color.r * 255.0f);
      pixels[base_index + 1] = static_cast<uint8_t>(color.g * 255.0f);
      pixels[base_index + 2] = static_cast<uint8_t>(color.b * 255.0f);
      pixels[base_index + 3] = static_cast<uint8_t>(color.a * 255.0f);
    }
  }
}
