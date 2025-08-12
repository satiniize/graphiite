#include "bitmap.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Helper functions for little-endian I/O
static void write_u16(std::ofstream &out, uint16_t val) {
  out.put(val & 0xFF);
  out.put((val >> 8) & 0xFF);
}

static void write_u32(std::ofstream &out, uint32_t val) {
  out.put(val & 0xFF);
  out.put((val >> 8) & 0xFF);
  out.put((val >> 16) & 0xFF);
  out.put((val >> 24) & 0xFF);
}

static uint16_t read_u16(std::ifstream &in) {
  uint8_t b0 = in.get();
  uint8_t b1 = in.get();
  return (b1 << 8) | b0;
}

static uint32_t read_u32(std::ifstream &in) {
  uint8_t b0 = in.get();
  uint8_t b1 = in.get();
  uint8_t b2 = in.get();
  uint8_t b3 = in.get();
  return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

bool Bitmap::dump(const std::string &filename) const {
  int width = width_;
  int height = height_;
  int row_size = ((width * 3 + 3) / 4) * 4;
  int padding = row_size - width * 3;
  int image_size = row_size * height;
  int file_size = 14 + 40 + image_size;

  std::ofstream out(filename, std::ios::binary);
  if (!out)
    return false;

  // BMP File Header (14 bytes)
  write_u16(out, 0x4D42);    // bfType 'BM'
  write_u32(out, file_size); // bfSize
  write_u16(out, 0);         // bfReserved1
  write_u16(out, 0);         // bfReserved2
  write_u32(out, 54);        // bfOffBits

  // DIB Header (BITMAPINFOHEADER, 40 bytes)
  write_u32(out, 40);         // biSize
  write_u32(out, width);      // biWidth
  write_u32(out, height);     // biHeight
  write_u16(out, 1);          // biPlanes
  write_u16(out, 24);         // biBitCount
  write_u32(out, 0);          // biCompression
  write_u32(out, image_size); // biSizeImage
  write_u32(out, 2835);       // biXPelsPerMeter
  write_u32(out, 2835);       // biYPelsPerMeter
  write_u32(out, 0);          // biClrUsed
  write_u32(out, 0);          // biClrImportant

  // Write pixel data (bottom-up)
  for (int y = 0; y < height; ++y) {
    int bmp_y = height - 1 - y;
    for (int x = 0; x < width; ++x) {
      uint32_t pixel = pixels_[bmp_y * width + x];
      uint8_t r = (pixel >> 24) & 0xFF;
      uint8_t g = (pixel >> 16) & 0xFF;
      uint8_t b = (pixel >> 8) & 0xFF;
      out.put(b);
      out.put(g);
      out.put(r);
    }
    for (int p = 0; p < padding; ++p) {
      out.put(0);
    }
  }

  out.close();
  return true;
}

bool Bitmap::load(const std::string &filename) {
  std::ifstream in(filename, std::ios::binary);
  if (!in)
    return false;

  // BMP Header
  if (read_u16(in) != 0x4D42)
    return false;             // 'BM'
  in.seekg(8, std::ios::cur); // skip file size and reserved
  uint32_t pixel_data_offset = read_u32(in);

  // DIB Header
  uint32_t dib_header_size = read_u32(in);
  if (dib_header_size < 40)
    return false; // Only support headers at least 40 bytes (BITMAPINFOHEADER or
                  // larger)
  uint32_t width = read_u32(in);
  uint32_t height = read_u32(in);
  uint16_t planes = read_u16(in);
  uint16_t bit_count = read_u16(in);
  uint32_t compression = read_u32(in);

  if (planes != 1 || bit_count != 24 || compression != 0 || width == 0 ||
      height == 0)
    return false;

  // Skip any extra DIB header bytes
  if (dib_header_size > 40) {
    in.seekg(dib_header_size - 40, std::ios::cur);
  }

  in.seekg(pixel_data_offset, std::ios::beg);

  int row_size = ((width * 3 + 3) / 4) * 4;
  int padding = row_size - width * 3;

  width_ = width;
  height_ = height;
  pixels_.assign(width * height, 0);

  for (uint32_t y = 0; y < height; ++y) {
    uint32_t bmp_y = height - 1 - y;
    for (uint32_t x = 0; x < width; ++x) {
      char pixel_buffer[3];
      in.read(pixel_buffer, 3);
      uint8_t b = static_cast<uint8_t>(pixel_buffer[0]);
      uint8_t g = static_cast<uint8_t>(pixel_buffer[1]);
      uint8_t r = static_cast<uint8_t>(pixel_buffer[2]);
      // If magenta (R=255, B=255), set alpha to 0x00, else 0xFF
      uint8_t a = (r == 255 && b == 255) ? 0x00 : 0xFF;
      set_pixel(x, bmp_y, (r << 24) | (g << 16) | (b << 8) | a);
    }
    in.ignore(padding);
  }

  in.close();

  return true;
}
