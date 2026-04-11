#include "image.hpp"
#include <math.h>

Image::Image(uint16_t width, uint16_t height, PixelFormat pixel_format) {
  int bytes = pixel_format == PixelFormat::RGBA8 ? 4 : 8;
  pixels.resize(width * height * bytes);
  this->pixel_format = pixel_format;
  this->width = width;
  this->height = height;
}

// sRGB Interpolation
// TODO: Can consider oklab interpolation
static Clay_Color lerp(Clay_Color a, Clay_Color b, float t) {
  return Clay_Color{
      a.r + (b.r - a.r) * t,
      a.g + (b.g - a.g) * t,
      a.b + (b.b - a.b) * t,
      a.a + (b.a - a.a) * t,
  };
}

// With the assumption gradient stops are ordered ascending
ImageGenerator Image::angular_gradient(std::vector<GradientStop> stops) {
  return [stops](float x, float y, float w, float h) -> Clay_Color {
    float angle = atan2(y - h / 2.0f, -x + w / 2.0f);
    float t = (angle + 3.141592654f) / (2.0f * 3.141592654f);
    GradientStop a = stops.back();
    GradientStop b = stops.back();
    b.position -= 1.0f;

    for (auto stop : stops) {
      a = b;
      b = stop;
      if (t <= b.position) {
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

ImageGenerator Image::linear_gradient(float angle,
                                      std::vector<GradientStop> stops) {
  return [angle, stops](float x, float y, float w, float h) -> Clay_Color {
    float t = (x - w / 2.0f) / w * cos(angle) - (y - h / 2.0f) / h * sin(angle);
    // TODO: Stretch t here, which is currently a unit circle
    t += 0.5f;
    GradientStop a = stops.front();
    GradientStop b = a;
    for (auto stop : stops) {
      a = b;
      b = stop;
      if (t <= b.position) {
        return lerp(a.color, b.color,
                    (t - a.position) / (b.position - a.position));
      }
    }
    return Clay_Color{t, t, t, 1.0f};
  };
}

void Image::fill(ImageGenerator generator) {
  int bytes = pixel_format == PixelFormat::RGBA8 ? 4 : 8;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      Clay_Color color = generator(x, y, width, height);
      int base_index = (y * width + x) * bytes;
      pixels[base_index + 0] = static_cast<uint8_t>(color.r);
      pixels[base_index + 1] = static_cast<uint8_t>(color.g);
      pixels[base_index + 2] = static_cast<uint8_t>(color.b);
      pixels[base_index + 3] = static_cast<uint8_t>(color.a);
    }
  }
}
