#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace FileDialog {
std::filesystem::path save_file(const char *title,
                                std::vector<const char *> extensions,
                                const char *description);
std::filesystem::path open_file(const char *title,
                                std::vector<const char *> extensions,
                                const char *description);
} // namespace FileDialog
