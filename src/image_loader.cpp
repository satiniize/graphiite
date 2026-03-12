#include "image_loader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "SDL3/SDL_log.h"
#include "image.hpp"
#include "turbojpeg.h"
#include <filesystem>
#include <fstream>
#include <vector>

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

Image load_with_turbojpeg(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
    SDL_Log("Invalid photo path");
    return {};
  }

  SDL_Log("Loading file: %s", path.c_str());

  std::ifstream jpeg_stream(path, std::ios::binary | std::ios::ate);
  if (!jpeg_stream.is_open()) {
    SDL_Log("ERROR: opening input file %s: %s", path.c_str(), strerror(errno));
    return {};
  }

  // Read jpeg into memory
  std::streampos size = jpeg_stream.tellg();
  if (size == 0) {
    SDL_Log("WARNING: Input file contains no data");
    return {};
  }
  jpeg_stream.seekg(0, std::ios::beg);
  size_t jpeg_size = static_cast<size_t>(size);
  std::vector<uint8_t> jpeg_buffer(jpeg_size);
  jpeg_stream.read(reinterpret_cast<char *>(jpeg_buffer.data()), jpeg_size);
  jpeg_stream.close();

  // Initialize TurboJPEG decompressor
  tjhandle turbojpeg_instance = tj3Init(TJINIT_DECOMPRESS);
  if (!turbojpeg_instance) {
    SDL_Log("ERROR: creating TurboJPEG instance");
    return {};
  }

  // Read JPEG header to get image info
  if (tj3DecompressHeader(turbojpeg_instance, jpeg_buffer.data(), jpeg_size) <
      0) {
    SDL_Log("ERROR: reading JPEG header for %s: %s", path.c_str(),
            tj3GetErrorStr(turbojpeg_instance));
    return {};
  }

  int width = tj3Get(turbojpeg_instance, TJPARAM_JPEGWIDTH);
  int height = tj3Get(turbojpeg_instance, TJPARAM_JPEGHEIGHT);
  int precision = tj3Get(turbojpeg_instance, TJPARAM_PRECISION);
  // int subsampling = tj3Get(turbojpeg_instance, TJPARAM_SUBSAMP);
  // int color_space = tj3Get(turbojpeg_instance, TJPARAM_COLORSPACE);

  int pixel_format = TJPF_RGBA;
  int output_channel = tjPixelSize[pixel_format];

  // 8 Bit
  if (precision > 8) {
    SDL_Log("ERROR: unsupported precision %d for JPEG image %s", precision,
            path.c_str());
    return {};
  }

  size_t output_size = width * height * output_channel;
  std::vector<uint8_t> pixel_data(output_size);
  if (tj3Decompress8(turbojpeg_instance, jpeg_buffer.data(), jpeg_size,
                     pixel_data.data(), 0, pixel_format) < 0) {
    SDL_Log("ERROR: decompressing 8-bit JPEG image %s: %s", path.c_str(),
            tj3GetErrorStr(turbojpeg_instance));
    return {};
  }

  Image image;
  image.width = width;
  image.height = height;
  image.pixels = pixel_data;

  // TODO: this is never called when returning early
  tj3Destroy(turbojpeg_instance);
  return image;
}
} // namespace ImageLoader
