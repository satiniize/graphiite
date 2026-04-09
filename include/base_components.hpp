#pragma once

#include "photo.hpp"
#include "texture.hpp"

#include <clay.h>
#include <string>
#include <vector>

namespace Components {
// TODO: Actually can consider passing in the straight string(?) for hashing
struct SliderContext {
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
// TODO: Automate id
void Slider(SliderContext *slider_context, uint32_t id, Texture &stroke_texture,
            Texture &fill_texture);
void Knob();
void Button(Clay_String label, Texture &stroke_texture, Texture &fill_texture,
            void button_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData),
            intptr_t userData = NULL);
void handle_photo_item_interaction(Clay_ElementId elementId,
                                   Clay_PointerData pointerInfo,
                                   intptr_t userData);
void PhotoItem(Texture &edge_sheen_data, Texture &bg_sheen_data,
               Texture &check_data, Photo &photo);
void PhotoGrid(Texture &edge_sheen_data, Texture &bg_sheen_data,
               Texture &check_data, std::vector<Photo> &photos,
               int photo_columns);
void Placeholder();
void Tally(Texture &edge_sheen_data, Texture &bg_sheen_data, Clay_String label);
// void BottomBar(
//     Texture &edge_sheen_data, Texture &bg_sheen_data, std::string
//     tally_label, void on_open_folder(Clay_ElementId elementId,
//     Clay_PointerData pointerInfo,
//                         intptr_t userData),
//     void on_finalize(Clay_ElementId elementId, Clay_PointerData pointerInfo,
//                      intptr_t userData),
//     void on_sort(Clay_ElementId elementId, Clay_PointerData pointerInfo,
//                  intptr_t userData),
//     void on_filter(Clay_ElementId elementId, Clay_PointerData pointerInfo,
//                    intptr_t userData),
//     intptr_t userData);
} // namespace Components
