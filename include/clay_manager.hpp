#pragma once
#include "clay.h"
#include "renderer.hpp"
#include <stdio.h>

// TODO: Make a renderer interface and an SDLGPURenderer implementation
// TODO: Clay itself is global scope, implement context switching here for
// safety
class ClayManager {
public:
  Renderer *renderer;

  ClayManager(Renderer *renderer, int width, int height);
  ~ClayManager() {};
  void begin_layout(Clay_Vector2 mouse_position, bool is_mouse_down,
                    Clay_Vector2 mouse_scroll);
  void end_layout();

  static void handle_clay_errors(Clay_ErrorData errorData) {
    // See the Clay_ErrorData struct for more information
    printf("%s", errorData.errorText.chars);
    switch (errorData.errorType) {
    case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_FUNCTION_NOT_PROVIDED:
      break;
    case CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED:
      break;
    case CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED:
      break;
    case CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED:
      break;
    case CLAY_ERROR_TYPE_DUPLICATE_ID:
      break;
    case CLAY_ERROR_TYPE_FLOATING_CONTAINER_PARENT_NOT_FOUND:
      break;
    case CLAY_ERROR_TYPE_PERCENTAGE_OVER_1:
      break;
    case CLAY_ERROR_TYPE_INTERNAL_ERROR:
      break;
    }
  };
  static Clay_Dimensions MeasureText(Clay_StringSlice text,
                                     Clay_TextElementConfig *config,
                                     void *userData) {
    Renderer &renderer = *(Renderer *)userData;
    float scalar = config->fontSize / renderer.default_font.sample_point_size;

    float line_height = renderer.default_font.get_line_height() * scalar;

    float width = 0.0f;
    for (int i = 0; i < text.length; i++) {
      int cp = static_cast<unsigned char>(text.chars[i]);
      auto it = renderer.default_font.glyph_metrics.find(cp);
      if (it == renderer.default_font.glyph_metrics.end())
        continue;
      width += it->second.advance * scalar;
    }

    return Clay_Dimensions{.width = width, .height = line_height};
  }
};
