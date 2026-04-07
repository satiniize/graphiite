#include "image.hpp"

Image::Image(uint16_t width, uint16_t height, PixelFormat pixel_format) {
  int bytes = pixel_format == PixelFormat::RGBA8 ? 4 : 8;
  pixels.resize(width * height * bytes);
  this->pixel_format = pixel_format;
  this->width = width;
  this->height = height;
}

static glm::vec4 lerp(glm::vec4 a, glm::vec4 b, float t) {
  return a + (b - a) * t;
}

// TODO: Add gradient stops
// With the assumption gradient stops are ordered ascending
ImageGenerator Image::angular_gradient(std::vector<GradientStop> stops) {
  return [stops](float x, float y, float w, float h) -> glm::vec4 {
    float angle = atan2(y - h / 2.0f, -x + w / 2.0f);
    float t = (angle + M_PI) / (2.0f * M_PI);
    GradientStop a = {.position = 0.0f,
                      .color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)};
    GradientStop b = stops.back();
    b.position -= 1.0f;

    for (auto stop : stops) {
      a = b;
      b = stop;
      if (t < b.position) {
        return lerp(a.color, b.color,
                    (t - a.position) / (b.position - a.position));
      }
    }

    a = b;
    b = stops.front();
    b.position += 1.0f;
    return lerp(a.color, b.color, (t - a.position) / (b.position - a.position));
  };
}

void Image::fill(ImageGenerator generator) {
  int bytes = pixel_format == PixelFormat::RGBA8 ? 4 : 8;
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
