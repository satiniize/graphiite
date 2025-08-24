#pragma once

#include "SDL3/SDL.h"
#include "SDL3_image/SDL_image.h"
#include "clay.h"
#include "renderer.hpp"
#include <iostream>
#include <unordered_map>

namespace ClayRenderer {
static int NUM_CIRCLE_SEGMENTS = 16;

static std::unordered_map<std::string, bool> id_has_geometry;

static void render_filled_rounded_rect(const SDL_FRect rect,
                                       const float corner_radius,
                                       const Clay_Color _color);

void render_commands(Renderer &renderer,
                     Clay_RenderCommandArray renderCommands);
} // namespace ClayRenderer
