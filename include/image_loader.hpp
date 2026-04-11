#pragma once

#include "image.hpp"
#include <filesystem>

namespace ImageLoader {
Image load(const std::filesystem::path &path);
Image load_with_turbojpeg(const std::filesystem::path &path,
                          bool is_thumbnail = false);
} // namespace ImageLoader
