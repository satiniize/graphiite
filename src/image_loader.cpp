#include "image_loader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "image.hpp"
#include <SDL3/SDL_log.h>
#include <filesystem>
#include <fstream>
#include <libraw/libraw.h>
#include <turbojpeg.h>
#include <vector>

namespace ImageLoader {
// TODO: Double check path and file sanitization
Image load(const std::filesystem::path &path) {
  static const int desired_channels = 4;

  int w, h, channels;
  // Maybe change this to ifstream
  unsigned char *pixels =
      stbi_load(path.c_str(), &w, &h, &channels, desired_channels);

  if (!pixels) {
    return {};
  }

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
    tj3Destroy(turbojpeg_instance);
    return {};
  }

  int width = tj3Get(turbojpeg_instance, TJPARAM_JPEGWIDTH);
  int height = tj3Get(turbojpeg_instance, TJPARAM_JPEGHEIGHT);
  int precision = tj3Get(turbojpeg_instance, TJPARAM_PRECISION);
  // int subsampling = tj3Get(turbojpeg_instance, TJPARAM_SUBSAMP);
  // int color_space = tj3Get(turbojpeg_instance, TJPARAM_COLORSPACE);

  int pixel_format = TJPF_RGBA;
  int output_channel = tjPixelSize[pixel_format];

  int num_scaling_factors;
  tjscalingfactor *factors = tj3GetScalingFactors(&num_scaling_factors);

  // Pick the smallest scaling factor for speed
  tjscalingfactor scaling_factor = {factors[num_scaling_factors - 1].num,
                                    factors[num_scaling_factors - 1].denom};
  tj3SetScalingFactor(turbojpeg_instance, scaling_factor);

  int scaled_width = TJSCALED(width, scaling_factor);
  int scaled_height = TJSCALED(height, scaling_factor);

  // 8 Bit
  if (precision > 8) {
    SDL_Log("ERROR: unsupported precision %d for JPEG image %s", precision,
            path.c_str());
    tj3Destroy(turbojpeg_instance);
    return {};
  }

  size_t output_size = scaled_width * scaled_height * output_channel;
  std::vector<uint8_t> pixel_data(output_size);
  if (tj3Decompress8(turbojpeg_instance, jpeg_buffer.data(), jpeg_size,
                     pixel_data.data(), 0, pixel_format) < 0) {
    SDL_Log("ERROR: decompressing 8-bit JPEG image %s: %s", path.c_str(),
            tj3GetErrorStr(turbojpeg_instance));
    tj3Destroy(turbojpeg_instance);
    return {};
  }

  Image image;
  image.width = scaled_width;
  image.height = scaled_height;
  image.pixels = pixel_data;

  tj3Destroy(turbojpeg_instance);
  return image;
}

Image load_with_libraw(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
    SDL_Log("Invalid photo path");
    return {};
  }

  SDL_Log("Loading file: %s", path.c_str());

  LibRaw processor;

  // sRGB output, 8bpp, camera white balance
  processor.imgdata.params.output_color = 1; // sRGB
  processor.imgdata.params.output_bps = 8;
  processor.imgdata.params.use_camera_wb = 1;
  processor.imgdata.params.no_auto_bright = 0;
  processor.imgdata.params.gamm[0] = 1.0 / 2.2;
  processor.imgdata.params.gamm[1] = 0.0;

  int ret;

  if ((ret = processor.open_file(path.c_str())) != LIBRAW_SUCCESS) {
    SDL_Log("ERROR: opening RAW file %s: %s", path.c_str(),
            libraw_strerror(ret));
    return {};
  }

  if ((ret = processor.unpack()) != LIBRAW_SUCCESS) {
    SDL_Log("ERROR: unpacking RAW file %s: %s", path.c_str(),
            libraw_strerror(ret));
    return {};
  }

  if ((ret = processor.dcraw_process()) != LIBRAW_SUCCESS) {
    SDL_Log("ERROR: processing RAW file %s: %s", path.c_str(),
            libraw_strerror(ret));
    return {};
  }

  libraw_processed_image_t *raw_image = processor.dcraw_make_mem_image(&ret);
  if (!raw_image) {
    SDL_Log("ERROR: making mem image for %s: %s", path.c_str(),
            libraw_strerror(ret));
    return {};
  }

  // raw_image->data is packed RGB (3 channels), convert to RGBA (4 channels)
  const int width = raw_image->width;
  const int height = raw_image->height;
  const size_t pixel_count = static_cast<size_t>(width) * height;

  std::vector<uint8_t> pixels(pixel_count * 4);

  const uint8_t *src = raw_image->data;
  uint8_t *dst = pixels.data();

  for (size_t i = 0; i < pixel_count; i++) {
    dst[0] = src[0]; // R
    dst[1] = src[1]; // G
    dst[2] = src[2]; // B
    dst[3] = 255;    // A
    src += 3;
    dst += 4;
  }

  LibRaw::dcraw_clear_mem(raw_image);

  Image image;
  image.width = width;
  image.height = height;
  image.pixels = std::move(pixels);

  return image;
}

} // namespace ImageLoader
