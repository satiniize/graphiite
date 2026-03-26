#include "photo.hpp"
#include "texture.hpp"

#include <clay.h>
#include <vector>

namespace Components {
struct SliderDragState {
  bool active = false;
  float *target = nullptr;
  Clay_ElementId id;
};
static SliderDragState g_slider_drag;

void slider_interaction(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                        intptr_t userData);
void UpdateSliderDrag(bool is_mouse_down, Clay_Vector2 pointerPosition);
void Slider(float *value, uint32_t id, Texture &bevel_texture);
void Knob();
void Button(Clay_String label,
            void button_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData),
            Texture &orange_bevel_texture, intptr_t userData = NULL);
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
void BottomBar(
    Texture &edge_sheen_data, Texture &bg_sheen_data, std::string tally_label,
    void on_open_folder(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                        intptr_t userData),
    void on_finalize(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                     intptr_t userData),
    void on_sort(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                 intptr_t userData),
    void on_filter(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                   intptr_t userData),
    intptr_t userData);
} // namespace Components
