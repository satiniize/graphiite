#pragma once
#include <clay.h>
#include <cstdint>

namespace Color {
inline constexpr Clay_Color GREY = {96, 96, 96, 255};
inline constexpr Clay_Color OFF_WHITE = {245, 245, 235, 255};
inline constexpr Clay_Color ORANGE = {255, 127, 0, 255};
inline constexpr Clay_Color ORANGE_HOVER = {255, 192, 95, 255};

inline constexpr Clay_Color BLACK = {12, 12, 12, 255};
inline constexpr Clay_Color LIGHT_GREY = {192, 192, 192, 255};
inline constexpr Clay_Color MIDDLE_GREY = {127, 127, 127, 255};
inline constexpr Clay_Color DARK_GREY = {63, 63, 63, 255};
inline constexpr Clay_Color WHITE = {255, 255, 255, 255};

inline constexpr Clay_Color TRANSPARENT = {255, 255, 255, 0};
inline constexpr Clay_Color SELECTED_GREEN = {127, 255, 0, 255};
} // namespace Color

namespace FontSize {
inline constexpr uint16_t SMALL = 16;
inline constexpr uint16_t MEDIUM = 20;
inline constexpr uint16_t LARGE = 28;
} // namespace FontSize
