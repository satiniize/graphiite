#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Minimal bitmap class for 24-bit BMP files
class Bitmap {
public:
  // Default constructor: width=0, height=0, pixels empty
  Bitmap() : width_(0), height_(0), pixels_() {}

  // Constructor with width and height
  Bitmap(int w, int h) : width_(w), height_(h), pixels_(w * h, 0) {}

  // Constructor from file path
  Bitmap(const std::string &path) : width_(0), height_(0), pixels_() {
    load(path);
  }

  // Load bitmap from file path, returns true on success
  bool load(const std::string &path);

  // Dump bitmap to file path, returns true on success
  bool dump(const std::string &path) const;

  // Getters
  int get_width() const { return width_; }
  int get_height() const { return height_; }
  const std::vector<uint32_t> &get_pixels() const { return pixels_; }

  // Set a pixel at (x, y) to value (no bounds check)
  void set_pixel(int x, int y, uint32_t value) {
    pixels_[y * width_ + x] = value;
  }

private:
  int width_;
  int height_;
  std::vector<uint32_t> pixels_; // RGBA8888 format (0xRRGGBBAA)
};
