#pragma once

#include <filesystem>

#include "../../texture.hpp"

struct Photo {
  Texture image_data;
  bool selected;
  std::filesystem::path file_path;
};
