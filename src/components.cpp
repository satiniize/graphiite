#include "components.hpp"
#include "theme.hpp"

static const float HANDLE_SIZE = 24.0f;
static const float TRACK_WIDTH = 300.0f;
// The usable range the handle center can travel
static const float TRAVEL = TRACK_WIDTH - 24;

void Components::slider_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME ||
      pointerInfo.state == CLAY_POINTER_DATA_PRESSED) {
    float *normalized = reinterpret_cast<float *>(userData);
    Clay_ElementData trackData = Clay_GetElementData(elementId);
    if (trackData.found) {
      float localX = pointerInfo.position.x - trackData.boundingBox.x;
      // Offset by half handle so value=0 means handle flush left,
      // value=1 means handle flush right
      float raw = (localX - HANDLE_SIZE / 2.0f) / TRAVEL;
      // Clamp to [0, 1]
      if (raw < 0.0f)
        raw = 0.0f;
      if (raw > 1.0f)
        raw = 1.0f;
      *normalized = raw;
    }
  }
}

void Components::Slider(float *value) {
  CLAY({
      // .id = CLAY_IDI("SliderTrack", id),
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_FIXED(TRACK_WIDTH),
                      .height = CLAY_SIZING_FIXED(40),
                  },
              .childAlignment =
                  {
                      .x = CLAY_ALIGN_X_CENTER,
                      .y = CLAY_ALIGN_Y_CENTER,
                  },
          },
  }) {
    Clay_OnHover(slider_interaction, reinterpret_cast<intptr_t>(value));

    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_FIXED(6),
                    },
            },
        .backgroundColor = Color::BLACK,
        .cornerRadius = CLAY_CORNER_RADIUS(3),
    }) {}

    // Pixel offset of the handle's left edge
    float offset = (*value) * TRAVEL;

    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_FIXED(24),
                        .height = CLAY_SIZING_FIXED(40),
                    },
                .padding = {.left = 2, .right = 2, .top = 2, .bottom = 2},
            },
        .backgroundColor = Color::BLACK,
        .cornerRadius = CLAY_CORNER_RADIUS(6),
        .floating =
            {
                .offset = Clay_Vector2{offset, 0.0f},
                .attachPoints =
                    {
                        .element = CLAY_ATTACH_POINT_LEFT_CENTER,
                        .parent = CLAY_ATTACH_POINT_LEFT_CENTER,
                    },
                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                .attachTo = CLAY_ATTACH_TO_PARENT,
            },
    }) {
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_CENTER,
                          .y = CLAY_ALIGN_Y_CENTER,
                      },
              },
          .backgroundColor = Color::DARK_GREY,
          .cornerRadius = CLAY_CORNER_RADIUS(6 - 2),
          .border =
              {
                  .color = Color::WHITE,
                  .width = CLAY_BORDER_ALL(2),
              },
      }) {
        CLAY({
            .layout =
                {
                    .sizing =
                        {
                            .width = CLAY_SIZING_FIXED(4),
                            .height = CLAY_SIZING_FIXED(24),
                        },
                },
            .backgroundColor = Color::WHITE,
            .cornerRadius = CLAY_CORNER_RADIUS(2),
        }) {}
      }
    }
  }
}

void Components::Button(Texture &edge_sheen_data, Texture &bg_sheen_data,
                        Clay_String label,
                        void button_interaction(Clay_ElementId elementId,
                                                Clay_PointerData pointerInfo,
                                                intptr_t userData),
                        intptr_t userData) {
  uint16_t button_height = 48;
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_FIT(),
                      .height =
                          CLAY_SIZING_FIXED(static_cast<float>(button_height)),
                  },
              .padding = CLAY_PADDING_ALL(4),
          },
      .backgroundColor = Color::WHITE,
      .cornerRadius = CLAY_CORNER_RADIUS(button_height / 2.0f),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = Color::BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
          },
  }) {
    Clay_OnHover(button_interaction, userData);
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                .padding =
                    {
                        .left = 16,
                        .right = 16,
                        .top = 0,
                        .bottom = 0,
                    },
                .childAlignment =
                    {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    },
            },
        .backgroundColor = Clay_Hovered() ? Color::WHITE : Color::LIGHT_GREY,
        .cornerRadius = CLAY_CORNER_RADIUS(button_height / 2.0f - 4.0f),
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
            },
    }) {
      CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                           .textColor = Color::WHITE,
                           .fontSize = FontSize::MEDIUM,
                           .wrapMode = CLAY_TEXT_WRAP_NONE,
                           .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                       }));
    }
  }
}

void Components::handle_photo_item_interaction(Clay_ElementId elementId,
                                               Clay_PointerData pointerInfo,
                                               intptr_t userData) {
  Photo *photo = (Photo *)userData;
  // Pointer state allows you to detect mouse down / hold / release
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    photo->selected = !photo->selected;
    // Do some click handling
    // NavigateTo(buttonData->link);
  }
}

void Components::PhotoItem(Texture &edge_sheen_data, Texture &bg_sheen_data,
                           Texture &check_data, Photo &photo) {
  uint16_t corner_radius = 16;
  uint16_t checkbox_corner_radius = 5;
  CLAY({
      .layout =
          {
              .sizing = {.width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_GROW(0)},
              .padding = CLAY_PADDING_ALL(3),
              .childGap = 8,
              .childAlignment =
                  {
                      .x = CLAY_ALIGN_X_CENTER,
                      .y = CLAY_ALIGN_Y_TOP,
                  },
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
          },
      .backgroundColor = photo.selected ? Color::SELECTED_GREEN : Color::WHITE,
      .cornerRadius = CLAY_CORNER_RADIUS(static_cast<float>(corner_radius)),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = Color::BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
          },
  }) {
    Clay_OnHover(handle_photo_item_interaction, (intptr_t)&photo);
    CLAY({
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(
                    corner_radius - 3 - checkbox_corner_radius)),
            },
        .backgroundColor = Color::WHITE,
        .cornerRadius =
            CLAY_CORNER_RADIUS(static_cast<float>(corner_radius - 3)),
        .aspectRatio =
            {
                .aspectRatio = (3.0f / 2.0f),
            },
        .image =
            {
                .imageData = static_cast<void *>(&photo.image_data),
            },
    }) {
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_GROW(0),
                      },
              },
      }) {}
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_FIXED(32),
                          .height = CLAY_SIZING_FIXED(32),
                      },
                  .padding = CLAY_PADDING_ALL(3),
              },
          .backgroundColor =
              photo.selected ? Color::SELECTED_GREEN : Color::WHITE,
          .cornerRadius =
              CLAY_CORNER_RADIUS(static_cast<float>(checkbox_corner_radius)),
          .image =
              {
                  .imageData = static_cast<void *>(&edge_sheen_data),
              },
          .border =
              {
                  .color = Color::BLACK,
                  .width =
                      {
                          .left = 2,
                          .right = 2,
                          .top = 2,
                          .bottom = 2,
                      },
              },
      }) {
        CLAY({
            .layout =
                {
                    .sizing =
                        {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_GROW(0),
                        },
                },
            .backgroundColor = Color::WHITE,
            .cornerRadius = CLAY_CORNER_RADIUS(
                static_cast<float>(checkbox_corner_radius - 3)),
            .image =
                {
                    .imageData = static_cast<void *>(&bg_sheen_data),
                },
        }) {
          CLAY({
              .layout =
                  {
                      .sizing =
                          {
                              .width = CLAY_SIZING_GROW(0),
                              .height = CLAY_SIZING_GROW(0),
                          },
                  },
              .backgroundColor =
                  photo.selected ? Color::SELECTED_GREEN : Color::TRANSPARENT,
              .image =
                  {
                      .imageData = static_cast<void *>(&check_data),
                  },
          }) {}
        }
      }
    }
  }
}

void Components::PhotoGrid(Texture &edge_sheen_data, Texture &bg_sheen_data,
                           Texture &check_data, std::vector<Photo> &photos,
                           int photo_columns) {
  // Photo grid calculation
  int image_counter = 0;
  // int photo_columns = renderer.width / image_minimum_width;
  // int photo_columns = 1080 / image_minimum_width;

  int num_images = std::size(photos);
  CLAY({
      .id = CLAY_ID("PhotoGrid"),
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_GROW(0),
                      .height = CLAY_SIZING_GROW(0),
                  },
          },
      .clip =
          {
              .vertical = true,
              .childOffset = Clay_GetScrollOffset(), // Somehow this function
                                                     // resets the scroll to 0
                                                     // when the overlay is
                                                     // first rendered
          },
  }) {
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                .padding = CLAY_PADDING_ALL(8),
                .childGap = 4,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
    }) {
      for (int i = 0; i < 16 && image_counter < num_images; i++) {
        CLAY({.layout = {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_FIT(0)},
                  .childGap = 4,
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              }}) {
          for (int i = 0; i < photo_columns; i++) {
            if (image_counter < num_images) {
              PhotoItem(edge_sheen_data, bg_sheen_data, check_data,
                        photos[image_counter]);
              image_counter++;
            } else {
              CLAY({
                  .layout =
                      {
                          .sizing = {.width = CLAY_SIZING_GROW(0),
                                     .height = CLAY_SIZING_GROW(0)},
                      },
              }) {}
            }
          }
        }
      }
    }
  }
}

void Components::Placeholder() {
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_GROW(),
                      .height = CLAY_SIZING_GROW(),
                  },
              .childAlignment =
                  {
                      .x = CLAY_ALIGN_X_CENTER,
                      .y = CLAY_ALIGN_Y_CENTER,
                  },
          },
  }) {
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_FIT(),
                        .height = CLAY_SIZING_FIT(),
                    },
            },
    }) {
      CLAY_TEXT(CLAY_STRING("Looks like you haven't opened a folder yet."),
                CLAY_TEXT_CONFIG({
                    .textColor = Color::WHITE,
                    .fontSize = FontSize::MEDIUM,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                }));
    }
  }
}

void Components::Tally(Texture &edge_sheen_data, Texture &bg_sheen_data,
                       Clay_String label) {
  uint16_t diameter = 48;
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_FIXED(static_cast<float>(diameter)),
                      .height = CLAY_SIZING_FIXED(static_cast<float>(diameter)),
                  },
              .padding = CLAY_PADDING_ALL(4),
          },
      .backgroundColor = Color::SELECTED_GREEN,
      .cornerRadius = CLAY_CORNER_RADIUS(diameter / 2.0f),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = Color::BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
          },
  }) {
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                .padding =
                    {
                        .left = 0,
                        .right = 0,
                        .top = 0,
                        .bottom = 0,
                    },
                .childAlignment =
                    {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    },
            },
        .backgroundColor = Clay_Hovered() ? Color::WHITE : Color::LIGHT_GREY,
        .cornerRadius = CLAY_CORNER_RADIUS(diameter / 2.0f - 4.0f),
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
            },
    }) {
      CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                           .textColor = Color::SELECTED_GREEN,
                           .fontSize = FontSize::MEDIUM,
                           .wrapMode = CLAY_TEXT_WRAP_NONE,
                           .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                       }));
    }
  }
}

void Components::BottomBar(
    Texture &edge_sheen_data, Texture &bg_sheen_data, std::string tally_label,
    void on_open_folder(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                        intptr_t userData),
    void on_finalize(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                     intptr_t userData),
    void on_sort(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                 intptr_t userData),
    void on_filter(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                   intptr_t userData),
    intptr_t userData) {
  float bottom_bar_corner_radius = 36.0f;
  CLAY({
      .id = CLAY_ID("BottomBar"),
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_GROW(0),
                      .height = CLAY_SIZING_FIT(0),
                  },
              .padding =
                  {
                      .left = 4,
                      .right = 4,
                      .top = 4,
                      .bottom = 4,
                  },
          },
      .backgroundColor = Color::BLACK,
      .cornerRadius =
          {
              .topLeft = bottom_bar_corner_radius,
              .topRight = bottom_bar_corner_radius,
              .bottomLeft = 0,
              .bottomRight = 0,
          },
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .floating =
          {
              .attachPoints =
                  {
                      .element = CLAY_ATTACH_POINT_CENTER_BOTTOM,
                      .parent = CLAY_ATTACH_POINT_CENTER_BOTTOM,
                  },
              .attachTo = CLAY_ATTACH_TO_PARENT,
          },
      .border =
          {
              .color = Color::WHITE,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
          },
  }) {
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                .padding =
                    {
                        .left = 4,
                        .right = 4,
                        .top = 4,
                        .bottom = 4,
                    },
            },
        .backgroundColor = Color::ORANGE,
        .cornerRadius =
            {
                .topLeft = bottom_bar_corner_radius - 4,
                .topRight = bottom_bar_corner_radius - 4,
                .bottomLeft = 0,
                .bottomRight = 0,
            },
    }) {
      // Left Align
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_LEFT,
                          .y = CLAY_ALIGN_Y_CENTER,
                      },
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              },
      }) {
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Open Folder"),
               on_open_folder, userData);
        CLAY({
            .layout =
                {
                    .sizing =
                        {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_GROW(0),
                        },
                },
        }) {}
        Tally(edge_sheen_data, bg_sheen_data,
              Clay_String{
                  .length = static_cast<int32_t>(tally_label.length()),
                  .chars = tally_label.c_str(),
              });
      }
      // Center Align
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_FIT(),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  .padding =
                      {
                          .left = 4,
                          .right = 4,
                      },
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_CENTER,
                      },
              },
      }) {
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Finalize"),
               on_finalize);
      }
      // Right Align
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  .childGap = 4,
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_RIGHT,
                      },
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              },
      }) {
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Sort"), on_sort);
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Filters"),
               on_filter);
      }
    }
  }
}
