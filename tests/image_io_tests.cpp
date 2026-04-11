#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

#include "image_io.hpp"

namespace fs = std::filesystem;

// Adjust these paths to point to actual test assets
static const fs::path kValidJpeg = "assets/xperia/bike.jpg";
static const fs::path kValidPng = "assets/test.png";
static const fs::path kInvalidFile = "assets/ColorChecker24_After_Nov2014.txt";
static const fs::path kMissing = "does_not_exist.jpg";

// ---------------------------------------------------------------------------
// ImageLoader::load
// ---------------------------------------------------------------------------

TEST(ImageLoaderLoad, LoadsValidJpeg) {
  auto img = ImageIO::load(kValidJpeg);
  EXPECT_GT(img.width, 0);
  EXPECT_GT(img.height, 0);
}

TEST(ImageLoaderLoad, LoadsValidPng) {
  auto img = ImageIO::load(kValidPng);
  EXPECT_GT(img.width, 0);
  EXPECT_GT(img.height, 0);
}

TEST(ImageLoaderLoad, ThrowsOnMissingFile) {
  EXPECT_THROW(ImageIO::load(kMissing), std::runtime_error);
}

TEST(ImageLoaderLoad, ThrowsOnInvalidFile) {
  EXPECT_THROW(ImageIO::load(kInvalidFile), std::runtime_error);
}

// ---------------------------------------------------------------------------
// ImageLoader::load_with_turbojpeg
// ---------------------------------------------------------------------------

TEST(ImageLoaderTurboJpeg, LoadsValidJpeg) {
  auto img = ImageIO::load_with_turbojpeg(kValidJpeg);
  EXPECT_GT(img.width, 0);
  EXPECT_GT(img.height, 0);
}

TEST(ImageLoaderTurboJpeg, LoadsThumbnail) {
  auto full = ImageIO::load_with_turbojpeg(kValidJpeg, false);
  auto thumb = ImageIO::load_with_turbojpeg(kValidJpeg, true);

  // Thumbnail should be smaller or equal in both dimensions
  EXPECT_LE(thumb.width, full.width);
  EXPECT_LE(thumb.height, full.height);
}

TEST(ImageLoaderTurboJpeg, ThrowsOnMissingFile) {
  EXPECT_THROW(ImageIO::load_with_turbojpeg(kMissing), std::runtime_error);
}

TEST(ImageLoaderTurboJpeg, ThrowsOnNonJpegFile) {
  // TurboJPEG only handles JPEG; passing a PNG should throw
  EXPECT_THROW(ImageIO::load_with_turbojpeg(kValidPng), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Consistency between load() and load_with_turbojpeg()
// ---------------------------------------------------------------------------

TEST(ImageLoaderConsistency, SameDimensionsForJpeg) {
  auto img1 = ImageIO::load(kValidJpeg);
  auto img2 = ImageIO::load_with_turbojpeg(kValidJpeg);

  EXPECT_EQ(img1.width, img2.width);
  EXPECT_EQ(img1.height, img2.height);
}
