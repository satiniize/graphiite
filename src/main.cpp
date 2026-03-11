#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

// Libraries
#include "SDL3/SDL.h"
// #include "SDL3/SDL_log.h"
// #include "SDL3/SDL_surface.h"
#include "turbojpeg.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "tinyfiledialogs.h"

#include "clay_renderer.hpp"
#include "config.hpp"
#include "renderer.hpp"

// Entities
#include "component_storage.hpp"
#include "entity_manager.hpp"

// Systems
#include "sprite_system.hpp"

// Components
#include "sprite_component.hpp"
#include "transform_component.hpp"

// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"

#include "image.hpp"
#include "image_loader.hpp"
#include "texture.hpp"

const Clay_Color COLOR_WHITE = {192, 192, 192, 255};
const Clay_Color COLOR_BLACK = {12, 12, 12, 255};
const Clay_Color COLOR_GREY = {96, 96, 96, 255};
const Clay_Color COLOR_DARK_GREY = {24, 24, 24, 255};
const Clay_Color COLOR_PURE_WHITE = {255, 255, 255, 255};

const Clay_Color COLOR_LIGHT_GREY = {160, 160, 160, 255};
const Clay_Color COLOR_TRANSPARENT = {255, 255, 255, 0};
const Clay_Color COLOR_SELECTED_GREEN = {127, 255, 0, 255};

uint16_t shut_up_data[1];

Renderer renderer;

Texture edge_sheen_data;
Texture carbon_fiber_data;
Texture vignette_data;
Texture bg_sheen_data;
Texture check_data;

struct Photo {
  Texture image_data;
  bool selected;
  std::filesystem::path file_path;
};

// std::string photos_root_path = "res/FUJI/";
std::vector<Photo> photos;
bool folder_opened = false;

std::string tally_label;

void handle_clay_errors(Clay_ErrorData errorData) {
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
}

void handle_photo_item_interaction(Clay_ElementId elementId,
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

void handle_finalize_button_interaction(Clay_ElementId elementId,
                                        Clay_PointerData pointerInfo,
                                        intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
    // folder_opened = false;
    // photos.clear();
  }
}

void handle_open_folder_button_interaction(Clay_ElementId elementId,
                                           Clay_PointerData pointerInfo,
                                           intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {

    char *lTheSelectFolderName = tinyfd_selectFolderDialog(
        "Choose a folder containing your photos", "./");

    if (!lTheSelectFolderName) {
      return;
    }

    // folder_opened = true;

    std::filesystem::path folder_path(lTheSelectFolderName);

    // TODO: for a later date
    // load_photos(folder_path);
  }
}

void handle_sort_button_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
  }
}

void handle_filters_button_interaction(Clay_ElementId elementId,
                                       Clay_PointerData pointerInfo,
                                       intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
  }
}

// TODO: Change hover to full photo rect, current selection is too small
inline void PhotoItem(Photo &photo) {
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
      .backgroundColor =
          photo.selected ? COLOR_SELECTED_GREEN : COLOR_PURE_WHITE,
      .cornerRadius = CLAY_CORNER_RADIUS(static_cast<float>(corner_radius)),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = COLOR_BLACK,
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
        .backgroundColor = COLOR_PURE_WHITE,
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
              photo.selected ? COLOR_SELECTED_GREEN : COLOR_PURE_WHITE,
          .cornerRadius =
              CLAY_CORNER_RADIUS(static_cast<float>(checkbox_corner_radius)),
          .image =
              {
                  .imageData = static_cast<void *>(&edge_sheen_data),
              },
          .border =
              {
                  .color = COLOR_BLACK,
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
            .backgroundColor = COLOR_PURE_WHITE,
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
                  photo.selected ? COLOR_SELECTED_GREEN : COLOR_TRANSPARENT,
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

void PhotoGrid(std::vector<Photo> &photos, int image_minimum_width) {
  // Photo grid calculation
  int image_counter = 0;
  int photo_columns = renderer.width / image_minimum_width;

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
              PhotoItem(photos[image_counter]);
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

void Placeholder() {
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
                    .textColor = COLOR_PURE_WHITE,
                    .fontSize = 24,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                }));
    }
  }
}

void Button(Clay_String label,
            void button_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData)) {
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
              .padding = CLAY_PADDING_ALL(3),
          },
      .backgroundColor = COLOR_PURE_WHITE,
      .cornerRadius = CLAY_CORNER_RADIUS(button_height / 2.0f),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = COLOR_BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
          },
  }) {
    Clay_OnHover(button_interaction, NULL);
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
        .backgroundColor = Clay_Hovered() ? COLOR_PURE_WHITE : COLOR_LIGHT_GREY,
        .cornerRadius = CLAY_CORNER_RADIUS(button_height / 2.0f - 3.0f),
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
            },
    }) {
      CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                           .textColor = {255, 255, 255, 255},
                           .fontSize = 24,
                           .wrapMode = CLAY_TEXT_WRAP_NONE,
                           .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                       }));
    }
  }
}

void Tally(Clay_String label) {
  uint16_t diameter = 48;
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_FIXED(static_cast<float>(diameter)),
                      .height = CLAY_SIZING_FIXED(static_cast<float>(diameter)),
                  },
              .padding = CLAY_PADDING_ALL(3),
          },
      .backgroundColor = COLOR_SELECTED_GREEN,
      .cornerRadius = CLAY_CORNER_RADIUS(diameter / 2.0f),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = COLOR_BLACK,
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
        .backgroundColor = Clay_Hovered() ? COLOR_PURE_WHITE : COLOR_LIGHT_GREY,
        .cornerRadius = CLAY_CORNER_RADIUS(diameter / 2.0f - 3.0f),
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
            },
    }) {
      CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                           .textColor = COLOR_SELECTED_GREEN,
                           .fontSize = 20,
                           .wrapMode = CLAY_TEXT_WRAP_NONE,
                           .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                       }));
    }
  }
}

void BottomBar() {
  float bottom_bar_corner_radius = 38.0f;
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
                      .left = 3,
                      .right = 3,
                      .top = 3,
                      .bottom = 0,
                  },
          },
      .backgroundColor = COLOR_PURE_WHITE,
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
              .color = COLOR_BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 0,
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
        .backgroundColor = COLOR_GREY,
        .cornerRadius =
            {
                .topLeft = bottom_bar_corner_radius - 3,
                .topRight = bottom_bar_corner_radius - 3,
                .bottomLeft = 0,
                .bottomRight = 0,
            },
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
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
        Button(CLAY_STRING("Open Folder"),
               handle_open_folder_button_interaction);
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
        Tally(Clay_String{
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
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_CENTER,
                      },
              },
      }) {
        Button(CLAY_STRING("Finalize"), handle_finalize_button_interaction);
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
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_RIGHT,
                      },
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              },
      }) {
        Button(CLAY_STRING("Sort"), handle_sort_button_interaction);
        Button(CLAY_STRING("Filters"), handle_filters_button_interaction);
      }
    }
  }
}

static inline Clay_Dimensions MeasureText(Clay_StringSlice text,
                                          Clay_TextElementConfig *config,
                                          void *userData) {
  float scalar = config->fontSize / renderer.font_sample_point_size;
  return Clay_Dimensions{.width = (float)text.length * renderer.glyph_size.x *
                                  scalar,
                         .height = (float)renderer.glyph_size.y * scalar};
}

bool init() {
  renderer.init();

  std::vector<std::string> loaded_sprite_paths;
  for (auto &[entity_id, sprite_component] : sprite_components) {
    auto it = std::find(loaded_sprite_paths.begin(), loaded_sprite_paths.end(),
                        sprite_component.path);

    if (it != loaded_sprite_paths.end()) {
      continue;
    }

    // SDL_Surface *image_data = IMG_Load(sprite_component.path.c_str());
    // if (!image_data) {
    //   SDL_Log("Failed to load image! %s", sprite_component.path.c_str());
    //   return false;
    // }

    // sprite_component.size = glm::ivec2(image_data->w, image_data->h);
    // TextureID texture_id = renderer.load_texture(image_data);
    // sprite_component.texture_id = texture_id;
    // SDL_DestroySurface(image_data);
    // loaded_sprite_paths.push_back(sprite_component.path);
  }
  return true;
}

bool loop() { return true; }

bool cleanup() {
  renderer.cleanup();
  return true;
}

// TODO: Give feedback after submitting finalize button
// TODO: Reset screen when finalize is pressed

Texture load_and_upload_texture(std::string path, bool tiling = false) {
  // int w, h, channels;
  // unsigned char *pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
  // TextureID texture_id = renderer.load_texture(pixels, w, h);
  // stbi_image_free(pixels);

  Image image = ImageLoader::load(path);
  TextureID texture_id =
      renderer.load_texture(image.pixels.data(), image.width, image.height);

  // This is used by clay renderer to tell renderer
  // - Which texture to use
  // - If the texture should be tiled
  // Technically once the texture is uploaded, path isn't necessary
  Texture texture_data;
  texture_data.path = path;
  texture_data.tiling = tiling;
  texture_data.id = texture_id;

  SDL_Log("Loaded texture %s with id %zu", path.c_str(), texture_id);

  return texture_data;
}

int main(int argc, char *argv[]) {
  // ECS
  // EntityManager entity_manager;

  // Player Amogus
  // EntityID amogus = entity_manager.create();
  // SpriteComponent sprite_component = {
  //     .path = "res/uv.bmp",
  // };
  // sprite_components[amogus] = sprite_component;
  // TransformComponent transform_component = {
  //     .position = glm::vec2(64.0f, 64.0f),
  //     .scale = glm::vec2(1.0f, 1.0f),
  // };
  // transform_components[amogus] = transform_component;

  // Init(texture uploading) must be after entities are created
  if (!init()) {
    return 1;
  }

  size_t total_memory_size = Clay_MinMemorySize();
  Clay_Arena clay_memory = Clay_CreateArenaWithCapacityAndMemory(
      total_memory_size, malloc(total_memory_size));
  Clay_Dimensions clay_dimensions = {.width = (float)WIDTH,
                                     .height = (float)HEIGHT};
  Clay_Context *clayContextBottom = Clay_Initialize(
      clay_memory, clay_dimensions, Clay_ErrorHandler{handle_clay_errors});

  Clay_SetMeasureTextFunction(MeasureText, nullptr);

  edge_sheen_data = load_and_upload_texture("res/edge_sheen.png");
  carbon_fiber_data = load_and_upload_texture("res/carbon_fiber.png", true);
  vignette_data = load_and_upload_texture("res/vignette.png");
  bg_sheen_data = load_and_upload_texture("res/bg_sheen.png");
  check_data = load_and_upload_texture("res/check.png");

  // Timing
  uint32_t prev_frame_tick = SDL_GetTicks();
  float physics_delta_time = 1.0f / physics_tick_rate;
  float process_delta_time = 0.0f;
  float accumulator = 0.0;

  int physics_frame_count = 0;
  int process_frame_count = 0;

  float time_scale = 1.0f;

  float scroll_speed = 6.0f;
  bool is_mouse_down = false;

  bool running = true;

  Clay_Vector2 mouse_position = {0.0f, 0.0f};

  // Do the one where it only updates when an event happens
  while (running) {
    // Calculate delta time
    uint32_t frame_tick = SDL_GetTicks();
    process_delta_time =
        static_cast<float>(frame_tick - prev_frame_tick) / 1000.0f;
    prev_frame_tick = frame_tick;
    accumulator += process_delta_time;

    ++process_frame_count;
    if (accumulator >= (physics_delta_time / time_scale)) {
      accumulator -= (physics_delta_time / time_scale);
      ++physics_frame_count;
      if (physics_frame_count >= static_cast<int>(physics_tick_rate)) {
        SDL_Log("FPS: %d", static_cast<int>(process_frame_count));
        physics_frame_count = 0;
        process_frame_count = 0;
      }
    }

    // Poll inputs
    Clay_Vector2 mouse_scroll = {0.0f, 0.0f};

    // const bool *keystate = SDL_GetKeyboardState(NULL);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        mouse_position.x = event.wheel.mouse_x;
        mouse_position.y = event.wheel.mouse_y;

        mouse_scroll.x = event.wheel.x * scroll_speed / renderer.viewport_scale;
        mouse_scroll.y = event.wheel.y * scroll_speed / renderer.viewport_scale;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        mouse_position.x = event.motion.x;
        mouse_position.y = event.motion.y;
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        is_mouse_down = true;
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        is_mouse_down = false;
        break;
      }
    }

    // TransformComponent &cursor_transform = transform_components[amogus];
    // cursor_transform.position = glm::vec2(mouse_position.x,
    // mouse_position.y);

    // Get tally count
    // This is technically a bit redundant
    // but is guaranteed to not be out of sync
    std::stringstream ss;
    // ss << get_selected_photos_count();
    tally_label = ss.str();

    renderer.begin_frame(); // Start here to update window dimensions for clay

    // Clay foreplay
    Clay_Dimensions clay_dimensions = {
        .width = static_cast<float>(renderer.width) / renderer.viewport_scale,
        .height =
            static_cast<float>(renderer.height) / renderer.viewport_scale};
    bool enable_drag_scrolling = false;
    float bottom_bar_height =
        Clay_GetElementData(CLAY_ID("BottomBar")).boundingBox.height;

    Clay_SetLayoutDimensions(clay_dimensions);
    Clay_SetPointerState(mouse_position, is_mouse_down);
    Clay_UpdateScrollContainers(enable_drag_scrolling, mouse_scroll,
                                process_delta_time);

    // Clay UI
    Clay_BeginLayout();
    CLAY({
        .id = CLAY_ID("Root"),
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(0),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        .backgroundColor = COLOR_PURE_WHITE,
        .image =
            {
                .imageData = static_cast<void *>(&carbon_fiber_data),
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
                  .childGap = 0,
                  .layoutDirection = CLAY_TOP_TO_BOTTOM,
              },
          .backgroundColor = COLOR_PURE_WHITE,
          .image =
              {
                  .imageData = static_cast<void *>(&vignette_data),
              },
      }) {
        // Image Grid
        if (folder_opened) {
          PhotoGrid(photos, 240 * renderer.viewport_scale);
        } else {
          Placeholder();
        }
        // Spacer
        CLAY({
            .layout =
                {
                    .sizing =
                        {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_FIXED(bottom_bar_height),
                        },
                },
        }) {}
      }
      // Bottom Bar
      BottomBar();
    }
    Clay_RenderCommandArray render_commands = Clay_EndLayout();
    ClayRenderer::render_commands(renderer, render_commands);
    SpriteSystem::draw_all(renderer);
    renderer.end_frame();
  }
  SDL_Log("Exiting...");
  cleanup();
  return 0;
}
