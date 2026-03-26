#pragma once

#include <filesystem>

#include "texture.hpp"

// TODO: Move this to photo-sorter
struct Photo {
  Texture image_data;
  bool selected;
  std::filesystem::path file_path;
};
