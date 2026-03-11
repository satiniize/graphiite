#pragma once

#include "image.hpp"
#include <filesystem>

namespace ImageLoader {
Image load(const std::filesystem::path &path);

}
