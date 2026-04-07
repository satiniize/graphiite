#include "clay_manager.hpp"
#include "clay.h"
#include "clay_renderer.hpp"
#include "components.hpp"

ClayManager::ClayManager(Renderer *renderer, int width, int height) {
  this->renderer = renderer;
  size_t total_memory_size = Clay_MinMemorySize();
  Clay_Arena clay_memory = Clay_CreateArenaWithCapacityAndMemory(
      total_memory_size, malloc(total_memory_size));
  Clay_Dimensions clay_dimensions = {.width = (float)width,
                                     .height = (float)height};
  Clay_Context *clayContextBottom =
      Clay_Initialize(clay_memory, clay_dimensions,
                      Clay_ErrorHandler{ClayManager::handle_clay_errors});
  Clay_SetMeasureTextFunction(ClayManager::MeasureText, this->renderer);
  Clay_SetDebugModeEnabled(true);
}

void ClayManager::begin_layout(Clay_Vector2 mouse_position, bool is_mouse_down,
                               Clay_Vector2 mouse_scroll) {
  Components::UpdateSliderDrag(is_mouse_down, mouse_position);

  Clay_Dimensions clay_dimensions = {
      .width = static_cast<float>(renderer->width) / renderer->viewport_scale,
      .height = static_cast<float>(renderer->height) / renderer->viewport_scale,
  };
  bool enable_drag_scrolling = false;
  float bottom_bar_height =
      Clay_GetElementData(CLAY_ID("BottomBar")).boundingBox.height;

  Clay_SetLayoutDimensions(clay_dimensions);
  Clay_SetPointerState(mouse_position, is_mouse_down);
  Clay_UpdateScrollContainers(enable_drag_scrolling, mouse_scroll, 1.0 / 60.0f);

  Clay_BeginLayout();
}

void ClayManager::end_layout() {
  Clay_RenderCommandArray render_commands = Clay_EndLayout();
  ClayRenderer::render_commands(*this->renderer, render_commands);
}
