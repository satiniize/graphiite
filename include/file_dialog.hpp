#pragma once

#include <filesystem>
#include <optional>
#include <span>

namespace FileDialog {
std::optional<std::filesystem::path>
save_file(std::string_view title, std::span<const std::string_view> extensions,
          std::string_view description);
std::optional<std::filesystem::path>
open_file(std::string_view title, std::span<const std::string_view> extensions,
          std::string_view description);
} // namespace FileDialog
