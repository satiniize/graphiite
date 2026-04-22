#pragma once

#include "texture.hpp"

#include <clay.h>
#include <string>
#include <vector>

namespace Components {
// TODO: Actually can consider passing in the straight string(?) for hashing
struct SliderContext { // TODO: Possibly rename this, as min and max will be
                       // reused for serializing
  float *value;
  float min_value;
  float max_value;
};
struct SliderDragState {
  bool active = false;
  SliderContext *slider_context = nullptr;
  Clay_ElementId id;
};

static SliderDragState g_slider_drag;

void slider_interaction(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                        intptr_t userData);
void UpdateSliderDrag(bool is_mouse_down, Clay_Vector2 pointerPosition);
void Slider(SliderContext *slider_context, uint32_t id);
void Knob();
void Button(Clay_String label, Texture &stroke_texture, Texture &fill_texture,
            void button_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData),
            intptr_t userData = 0, bool disabled = false);
} // namespace Components
