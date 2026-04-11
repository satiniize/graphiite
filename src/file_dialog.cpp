#include "file_dialog.hpp"

#include <vector>

#include <tinyfiledialogs.h>

std::optional<std::filesystem::path>
FileDialog::save_file(std::string_view title,
                      std::span<const std::string_view> extensions,
                      std::string_view description) {
  std::vector<const char *> filters;
  filters.reserve(extensions.size());
  for (auto &ext : extensions)
    filters.push_back(ext.data());

  const char *path = tinyfd_saveFileDialog(title.data(), "./",
                                           static_cast<int>(filters.size()),
                                           filters.data(), description.data());

  if (!path)
    return std::nullopt;
  return std::filesystem::path(path);
}

std::optional<std::filesystem::path>
FileDialog::open_file(std::string_view title,
                      std::span<const std::string_view> extensions,
                      std::string_view description) {
  std::vector<const char *> filters;
  filters.reserve(extensions.size());
  for (auto &ext : extensions)
    filters.push_back(ext.data());

  const char *path = tinyfd_openFileDialog(
      title.data(), "./", static_cast<int>(filters.size()), filters.data(),
      description.data(), 0);

  if (!path)
    return std::nullopt;
  return std::filesystem::path(path);
}
