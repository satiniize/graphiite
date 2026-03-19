#include "photo.hpp"
#include "texture.hpp"

#include <clay.h>
#include <vector>

namespace Components {
void slider_interaction(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                        intptr_t userData);
void Slider(float *value);
void Button(Texture &edge_sheen_data, Texture &bg_sheen_data, Clay_String label,
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
