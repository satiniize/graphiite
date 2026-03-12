#pragma once

#include "image.hpp"
#include <filesystem>

namespace ImageLoader {
Image load(const std::filesystem::path &path);
Image load_with_turbojpeg(const std::filesystem::path &path);
Image load_with_libraw(const std::filesystem::path &path);
} // namespace ImageLoader
