#pragma once

#include "image.hpp"
#include <filesystem>

namespace ImageIO {
Image load(const std::filesystem::path &path);
Image load_with_turbojpeg(const std::filesystem::path &path,
                          bool is_thumbnail = false);
void save(const std::filesystem::path &path, Image image);
} // namespace ImageIO
