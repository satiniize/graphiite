#pragma once
#include <clay.h>
#include <cstdint>

namespace Color {
inline constexpr Clay_Color LIGHT_FILL_HIGH = {207, 207, 207, 255};
inline constexpr Clay_Color LIGHT_FILL_LOW = {143, 143, 143, 255};
inline constexpr Clay_Color LIGHT_STROKE_HIGH = {255, 255, 255, 255};
inline constexpr Clay_Color LIGHT_STROKE_LOW = {191, 191, 191, 255};

inline constexpr Clay_Color GREY = {72, 72, 72, 255};
inline constexpr Clay_Color OFF_WHITE = {245, 245, 235, 255};
inline constexpr Clay_Color ORANGE = {255, 127, 0, 255};
inline constexpr Clay_Color ORANGE_HOVER = {255, 192, 95, 255};

inline constexpr Clay_Color BLACK = {12, 12, 12, 255};
inline constexpr Clay_Color LIGHT_GREY = {192, 192, 192, 255};
inline constexpr Clay_Color MIDDLE_GREY = {127, 127, 127, 255};
inline constexpr Clay_Color DARK_GREY = {47, 47, 47, 255};
inline constexpr Clay_Color WHITE = {255, 255, 255, 255};

inline constexpr Clay_Color TRANSPARENT = {255, 255, 255, 0};
inline constexpr Clay_Color SELECTED_GREEN = {127, 255, 0, 255};

inline constexpr Clay_Color WHITE100 = {255, 255, 255, 255};
inline constexpr Clay_Color WHITE80 = {230, 230, 230, 255};
inline constexpr Clay_Color WHITE60 = {204, 204, 204, 255};

} // namespace Color

namespace FontSize {
inline constexpr uint16_t SMALL = 8;
inline constexpr uint16_t MEDIUM = 18;
inline constexpr uint16_t LARGE = 36;
} // namespace FontSize

struct ButtonDefaults {
  Clay_Color fill_color = Color::LIGHT_FILL_HIGH;
  Clay_Color stroke_color = Color::LIGHT_STROKE_HIGH;
};
