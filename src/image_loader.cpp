#include "image_loader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ImageLoader {
Image load(const std::filesystem::path &path) {
  static const int desired_channels = 4;

  int w, h, channels;
  unsigned char *pixels =
      stbi_load(path.c_str(), &w, &h, &channels, desired_channels);

  Image image;
  image.width = w;
  image.height = h;
  image.channels = channels;
  image.pixels =
      std::vector<uint8_t>(pixels, pixels + w * h * desired_channels);
  stbi_image_free(pixels); // free stbi's buffer immediately after copy

  return image;
}
} // namespace ImageLoader
