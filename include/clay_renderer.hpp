#pragma once

#include <unordered_map>

#include "clay.h"
#include "renderer.hpp"

// struct ImageData {
//   std::string path;
//   bool tiling;
//   size_t id;
// };

namespace ClayRenderer {
static int NUM_CIRCLE_SEGMENTS = 16;

static std::unordered_map<std::string, bool> id_has_geometry;

static void render_filled_rounded_rect(const SDL_FRect rect,
                                       const float corner_radius,
                                       const Clay_Color _color);

void render_commands(Renderer &renderer,
                     Clay_RenderCommandArray renderCommands);
} // namespace ClayRenderer
