#pragma once
#include "clay.h"
#include <cstdint>

namespace Color {
inline constexpr Clay_Color WHITE = {192, 192, 192, 255};
inline constexpr Clay_Color BLACK = {12, 12, 12, 255};
inline constexpr Clay_Color GREY = {96, 96, 96, 255};
inline constexpr Clay_Color DARK_GREY = {24, 24, 24, 255};
inline constexpr Clay_Color PURE_WHITE = {255, 255, 255, 255};

inline constexpr Clay_Color LIGHT_GREY = {160, 160, 160, 255};
inline constexpr Clay_Color TRANSPARENT = {255, 255, 255, 0};
inline constexpr Clay_Color SELECTED_GREEN = {127, 255, 0, 255};
} // namespace Color

namespace FontSize {
inline constexpr uint16_t SMALL = 16;
inline constexpr uint16_t MEDIUM = 28;
inline constexpr uint16_t LARGE = 28;
} // namespace FontSize
