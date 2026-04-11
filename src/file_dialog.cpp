#include "file_dialog.hpp"

#include <tinyfiledialogs.h>

std::filesystem::path
FileDialog::save_file(const char *title, std::vector<const char *> extensions,
                      const char *description) {
  const char *path =
      tinyfd_saveFileDialog(title, "./", static_cast<int>(extensions.size()),
                            extensions.data(), description);
  if (!path)
    return std::filesystem::path{};
  return std::filesystem::path(path);
}

std::filesystem::path
FileDialog::open_file(const char *title, std::vector<const char *> extensions,
                      const char *description) {
  const char *path =
      tinyfd_openFileDialog(title, "./", static_cast<int>(extensions.size()),
                            extensions.data(), description, 0);
  if (!path)
    return std::filesystem::path{};
  return std::filesystem::path(path);
}
