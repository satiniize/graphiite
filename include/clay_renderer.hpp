#pragma once

#include "clay.h"
#include "renderer.hpp"

namespace ClayRenderer {
void render_commands(Renderer &renderer,
                     Clay_RenderCommandArray renderCommands);
} // namespace ClayRenderer
