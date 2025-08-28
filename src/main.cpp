#include "SDL3/SDL_log.h"
#include "SDL3/SDL_surface.h"
#include "glm/common.hpp"
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <random>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include "clay_renderer.hpp"
#include "config.hpp"
#include "glm/glm.hpp"
#include "renderer.hpp"

// Entities
#include "component_storage.hpp"
#include "entity_manager.hpp"

// Systems
#include "sprite_system.hpp"

// Components
#include "sprite_component.hpp"
#include "transform_component.hpp"

const Clay_Color COLOR_WHITE = {192, 192, 192, 255};
const Clay_Color COLOR_BLACK = {12, 12, 12, 255};
const Clay_Color COLOR_GREY = {40, 40, 40, 255};
const Clay_Color COLOR_DARK_GREY = {24, 24, 24, 255};
const Clay_Color COLOR_PURE_WHITE = {255, 255, 255, 255};

const Clay_Color COLOR_LIGHT_GREY = {160, 160, 160, 255};
const Clay_Color COLOR_TRANSPARENT = {255, 255, 255, 0};
const Clay_Color COLOR_SELECTED_GREEN = {127, 255, 0, 255};

uint16_t shut_up_data[1];
std::string test_image_path = "res/uv.bmp";
std::string carbon_fiber_path = "res/carbon_fiber.png";

Renderer renderer;

ImageData edge_sheen_data;
ImageData carbon_fiber_data;
ImageData vignette_data;
ImageData bg_sheen_data;
ImageData check_data;

struct Photo {
  ImageData image_data;
  bool selected;
};

std::vector<Photo> photos;

inline void DropDownMenuSeperator() {
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_GROW(0),
                      .height = CLAY_SIZING_FIT(0),
                  },
              .padding =
                  {
                      .left = 0,
                      .right = 0,
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
                        .height = CLAY_SIZING_FIXED(1),
                    },
            },
        .backgroundColor = COLOR_GREY,
    }) {}
  }
}

inline void DropDownMenuButton(Clay_String label) {
  CLAY({
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
              .childAlignment =
                  {
                      .x = CLAY_ALIGN_X_LEFT,
                      .y = CLAY_ALIGN_Y_CENTER,
                  },
          },
      .backgroundColor = Clay_Hovered() ? COLOR_GREY : COLOR_DARK_GREY,
      .cornerRadius =
          {
              .topLeft = 3,
              .topRight = 3,
              .bottomLeft = 3,
              .bottomRight = 3,
          },
  }) {
    CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                         .textColor = {255, 255, 255, 255},
                         .fontSize = 12,
                     }));
  }
}

inline void SessionDropdownMenu() {
  DropDownMenuButton(CLAY_STRING("New Session"));
  DropDownMenuButton(CLAY_STRING("Open Session..."));
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("Save Session"));
  DropDownMenuButton(CLAY_STRING("Save Session As..."));
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("Close Session"));
  DropDownMenuButton(CLAY_STRING("Exit"));
}

inline void EditDropdownMenu() {
  DropDownMenuButton(CLAY_STRING("Undo"));
  DropDownMenuButton(CLAY_STRING("Redo"));
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("Cut"));
  DropDownMenuButton(CLAY_STRING("Copy"));
  DropDownMenuButton(CLAY_STRING("Paste"));
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("Delete Selected"));
  DropDownMenuButton(CLAY_STRING("Select All"));
}

inline void ViewDropdownMenu() {
  DropDownMenuButton(CLAY_STRING("Zoom In"));
  DropDownMenuButton(CLAY_STRING("Zoom Out"));
  DropDownMenuButton(CLAY_STRING("Actual Size"));
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("Show Grid"));   // Toggle or submenu
  DropDownMenuButton(CLAY_STRING("Show Rulers")); // Toggle or submenu
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("Fullscreen"));
  DropDownMenuButton(CLAY_STRING("Preferences..."));
}

inline void HelpDropdownMenu() {
  DropDownMenuButton(CLAY_STRING("View Help"));
  DropDownMenuButton(CLAY_STRING("Keyboard Shortcuts"));
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("Check for Updates..."));
  DropDownMenuSeperator();
  DropDownMenuButton(CLAY_STRING("About PhotoSorter"));
}

inline void MenuBarButton(Clay_String label, void (*dropdown_menu)()) {
  CLAY({
      .id = CLAY_SIDI(label, 0),
      .layout =
          {
              .sizing = {.width = CLAY_SIZING_FIT(0),
                         .height = CLAY_SIZING_FIT(0)},
              .padding =
                  {
                      .left = 4,
                      .right = 4,
                      .top = 4,
                      .bottom = 4,
                  },
              .childAlignment =
                  {
                      .x = CLAY_ALIGN_X_CENTER,
                      .y = CLAY_ALIGN_Y_CENTER,
                  },
          },
      .backgroundColor = Clay_Hovered() ? COLOR_GREY : COLOR_DARK_GREY,
      .cornerRadius =
          {
              .topLeft = 3,
              .topRight = 3,
              .bottomLeft = 3,
              .bottomRight = 3,
          },
  }) {
    CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                         .textColor = {255, 255, 255, 255},
                         .fontSize = 12,
                     }));

    bool file_menu_visible =
        Clay_PointerOver(Clay_GetElementIdWithIndex(label, 0)) ||
        Clay_PointerOver(Clay_GetElementIdWithIndex(label, 1));

    if (file_menu_visible) {
      CLAY({
          .id = CLAY_SIDI(label, 1),
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_FIXED(192),
                          .height = CLAY_SIZING_FIT(0),
                      },
                  .padding = CLAY_PADDING_ALL(3),
              },
          .backgroundColor = COLOR_WHITE,
          .cornerRadius =
              {
                  .topLeft = 8,
                  .topRight = 8,
                  .bottomLeft = 8,
                  .bottomRight = 8,
              },
          .floating =
              {
                  .zIndex = 1,
                  .attachPoints =
                      {
                          .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
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
                    .padding = CLAY_PADDING_ALL(2),
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
            .backgroundColor = COLOR_DARK_GREY,
            .cornerRadius =
                {
                    .topLeft = 5,
                    .topRight = 5,
                    .bottomLeft = 5,
                    .bottomRight = 5,
                },
        }) {
          dropdown_menu();
        }
      }
    }
  }
}

std::string edge_sheen_path = "res/edge_sheen.png";

void HandleButtonInteraction(Clay_ElementId elementId,
                             Clay_PointerData pointerInfo, intptr_t userData) {
  Photo *photo = (Photo *)userData;
  // Pointer state allows you to detect mouse down / hold / release
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    photo->selected = !photo->selected;
    // Do some click handling
    // NavigateTo(buttonData->link);
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
    Clay_OnHover(HandleButtonInteraction, (intptr_t)&photo);
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
      // CLAY_TEXT(CLAY_STRING("IMGTEST.JPG"),
      //           CLAY_TEXT_CONFIG({
      //               .textColor = {255, 255, 255, 255},
      //               .fontSize = 16,
      //           }));
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
                  photo.selected ? COLOR_PURE_WHITE : COLOR_TRANSPARENT,
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

void handle_clay_errors(Clay_ErrorData errorData) {
  // See the Clay_ErrorData struct for more information
  printf("%s", errorData.errorText.chars);
  switch (errorData.errorType) {
    // etc
  }
}

static inline Clay_Dimensions MeasureText(Clay_StringSlice text,
                                          Clay_TextElementConfig *config,
                                          void *userData) {
  float scalar = config->fontSize / renderer.font_sample_point_size;
  return (Clay_Dimensions){.width = (float)text.length * renderer.glyph_size.x *
                                    scalar,
                           .height = (float)renderer.glyph_size.y * scalar};
}

void ImageGrid() {}

void Button(Clay_String label) {
  // Finalize button
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
                           .fontSize = 20,
                       }));
    }
  }
}

void Tally(Clay_String label) {
  // Finalize button
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
                       }));
    }
  }
}

int get_selected_photos_count() {
  int count = 0;
  for (auto photo : photos) {
    if (photo.selected) {
      count++;
    }
  }
  return count;
}

bool init() {
  renderer.init();
  // For every sprite, add their paths to an array
  // Send this array to renderer to load data and transfer to gpu
  std::vector<std::string> loaded_sprite_paths;
  for (auto &[entity_id, sprite_component] : sprite_components) {
    auto it = std::find(loaded_sprite_paths.begin(), loaded_sprite_paths.end(),
                        sprite_component.path);

    if (it != loaded_sprite_paths.end()) {
      continue;
    }

    SDL_Surface *image_data = IMG_Load(sprite_component.path.c_str());
    if (image_data == NULL) {
      SDL_Log("Failed to load image! %s", sprite_component.path.c_str());
      return false;
    }

    renderer.load_texture(sprite_component.path, image_data);
    SDL_DestroySurface(image_data);
    loaded_sprite_paths.push_back(sprite_component.path);
  }
  return true;
}

bool loop() { return true; }

bool cleanup() {
  renderer.cleanup();
  return true;
}

int main(int argc, char *argv[]) {
  EntityManager entity_manager;

  // Player Amogus
  EntityID amogus = entity_manager.create();
  SpriteComponent sprite_component = {
      .path = "res/uv.bmp",
  };
  sprite_components[amogus] = sprite_component;
  // TODO: Get size of image
  TransformComponent transform_component = {
      .position = glm::vec2(64.0f, 64.0f),
      .scale = glm::vec2(8.0f, 8.0f),
  };
  transform_components[amogus] = transform_component;

  // Init(texture uploading) must be after entities are created
  if (!init()) {
    return 1;
  }

  uint32_t prev_frame_tick = SDL_GetTicks();
  float physics_delta_time = 1.0f / physics_tick_rate;
  float process_delta_time = 0.0f;
  float accumulator = 0.0;

  int physics_frame_count = 0;
  int process_frame_count = 0;
  float time_scale = 1.0f;

  bool running = true;

  uint64_t total_memory_size = Clay_MinMemorySize();
  Clay_Arena clay_memory = Clay_CreateArenaWithCapacityAndMemory(
      total_memory_size, malloc(total_memory_size));
  Clay_Dimensions clay_dimensions = {.width = (float)WIDTH,
                                     .height = (float)HEIGHT};
  Clay_Context *clayContextBottom = Clay_Initialize(
      clay_memory, clay_dimensions, (Clay_ErrorHandler){handle_clay_errors});
  shut_up_data[0] = 1;
  Clay_SetMeasureTextFunction(MeasureText, (void *)shut_up_data);

  std::string photos_root_path = "res/FUJI/";

  if (!std::filesystem::exists(photos_root_path) &&
      std::filesystem::is_directory(photos_root_path)) {
    SDL_Log("Invalid photo path");
    return 1;
  }

  // std::vector<ImageData> photo_paths;

  for (const std::filesystem::directory_entry &entry :
       std::filesystem::directory_iterator(photos_root_path)) {
    if (entry.is_regular_file()) {
      std::cout << "File: " << entry.path() << std::endl;

      Photo photo{};
      ImageData photo_image_data{};
      photo_image_data.path = entry.path().string();
      photo_image_data.tiling = false;

      photo.image_data = photo_image_data;
      photo.selected = false;

      photos.push_back(photo);
      // photo_paths.push_back(image_data);
      SDL_Log("Start IMG_Load");
      SDL_Surface *image_data = IMG_Load(entry.path().c_str());
      SDL_Log("End IMG_Load");
      if (image_data == NULL) {
        SDL_Log("Failed to load image! %s", entry.path().c_str());
      }

      SDL_Log("Start downsample");
      int downsample_factor = 8;
      int width = image_data->w / downsample_factor;
      int height = image_data->h / downsample_factor;
      SDL_Surface *downsampled =
          SDL_CreateSurface(width, height, image_data->format);

      SDL_Rect src_rect = {0, 0, image_data->w, image_data->h};
      SDL_Rect dst_rect = {0, 0, width, height};
      SDL_BlitSurfaceScaled(image_data, &src_rect, downsampled, &dst_rect,
                            SDL_SCALEMODE_LINEAR);

      SDL_Log("End downsample");
      SDL_Log("Start GPU upload");
      renderer.load_texture(entry.path(), downsampled);
      SDL_Log("End GPU upload");
      SDL_DestroySurface(image_data);
      SDL_DestroySurface(downsampled);
    }
  }

  int num_images = std::size(photos);
  bool is_mouse_down = false;

  // Clay_Vector2 margin_container_scroll = {0.0, 0.0};
  // float scroll_speed = 4.0f;
  SDL_Surface *edge_sheen = IMG_Load("res/edge_sheen.png");
  renderer.load_texture("res/edge_sheen.png", edge_sheen);
  SDL_DestroySurface(edge_sheen);

  edge_sheen_data.path = "res/edge_sheen.png";
  edge_sheen_data.tiling = false;

  SDL_Surface *carbon_fiber = IMG_Load("res/carbon_fiber.png");
  renderer.load_texture("res/carbon_fiber.png", carbon_fiber);
  SDL_DestroySurface(carbon_fiber);

  carbon_fiber_data.path = "res/carbon_fiber.png";
  carbon_fiber_data.tiling = true;

  SDL_Surface *vignette = IMG_Load("res/vignette.png");
  renderer.load_texture("res/vignette.png", vignette);
  SDL_DestroySurface(vignette);

  vignette_data.path = "res/vignette.png";
  vignette_data.tiling = false;

  SDL_Surface *bg_sheen = IMG_Load("res/bg_sheen.png");
  renderer.load_texture("res/bg_sheen.png", bg_sheen);
  SDL_DestroySurface(bg_sheen);

  bg_sheen_data.path = "res/bg_sheen.png";
  bg_sheen_data.tiling = false;

  SDL_Surface *check = IMG_Load("res/check.png");
  renderer.load_texture("res/check.png", check);
  SDL_DestroySurface(check);

  check_data.path = "res/check.png";
  check_data.tiling = false;

  float scroll_speed = 6.0f;

  while (running) {
    std::stringstream ss;
    ss << get_selected_photos_count();
    std::string str = ss.str();

    const char *c_str = str.c_str();
    int len = strlen(c_str);

    uint32_t frame_tick = SDL_GetTicks();
    process_delta_time =
        static_cast<float>(frame_tick - prev_frame_tick) / 1000.0f;
    accumulator += process_delta_time;
    prev_frame_tick = frame_tick;

    glm::vec2 cursor_pos;
    float cursor_x;
    float cursor_y;

    Clay_Vector2 mouse_scroll = {0.0f, 0.0f};
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        cursor_pos.x = event.wheel.mouse_x;
        cursor_pos.y = event.wheel.mouse_y;

        mouse_scroll.x = event.wheel.x * scroll_speed / renderer.viewport_scale;
        mouse_scroll.y = event.wheel.y * scroll_speed / renderer.viewport_scale;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        cursor_pos.x = event.motion.x;
        cursor_pos.y = event.motion.y;
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        is_mouse_down = true;
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        is_mouse_down = false;
        break;
      }
    }

    Clay_Vector2 mouse_position = {cursor_pos.x, cursor_pos.y};

    // const bool *keystate = SDL_GetKeyboardState(NULL);

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

    TransformComponent &cursor_transform = transform_components[amogus];
    cursor_transform.position = glm::vec2(cursor_pos.x, cursor_pos.y);

    int image_counter = 0;

    renderer.begin_frame();

    Clay_Dimensions clay_dimensions = {
        .width = static_cast<float>(renderer.width) /
                 static_cast<float>(renderer.viewport_scale),
        .height = static_cast<float>(renderer.height) /
                  static_cast<float>(renderer.viewport_scale)};

    // Photo grid calculation
    int image_minimum_width = 240 * renderer.viewport_scale;
    int photo_columns = renderer.width / image_minimum_width;

    bool enable_drag_scrolling = false;

    // Clay foreplay
    Clay_SetLayoutDimensions(clay_dimensions);
    Clay_SetPointerState(mouse_position, is_mouse_down);
    Clay_UpdateScrollContainers(enable_drag_scrolling, mouse_scroll,
                                process_delta_time);

    uint16_t spacing = 5;
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
      // Image Grid
      CLAY({
          .id = CLAY_ID("ImageGrid"),
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  // .layoutDirection = CLAY_TOP_TO_BOTTOM,
              },
          .backgroundColor = COLOR_PURE_WHITE,
          .image = // TODO: Move this after root
          {
              .imageData = static_cast<void *>(&vignette_data),
          },
          .clip =
              {
                  .vertical = true,
                  .childOffset =
                      Clay_GetScrollOffset(), // Somehow this function
                                              // resets the scroll to 0 when
                                              // the overlay is first
                                              // rendered
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
          // Spacer
          CLAY({
              .layout =
                  {
                      .sizing =
                          {
                              .width = CLAY_SIZING_GROW(0),
                              .height = CLAY_SIZING_FIXED(52),
                          },
                  },
          }) {}
        }
      }
      // Bottom Bar
      float bottom_bar_corner_radius = 38.0f;
      CLAY({
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
            .backgroundColor = COLOR_LIGHT_GREY,
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
            Button(CLAY_STRING("Open Folder"));
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
            Tally((Clay_String){
                // .isStaticallyAllocated = false,
                .length = len,
                .chars = c_str,
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
            Button(CLAY_STRING("Finalize"));
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
            Button(CLAY_STRING("Sort"));
            Button(CLAY_STRING("Filters"));
          }
        }
      }
    }
    Clay_RenderCommandArray render_commands = Clay_EndLayout();

    // TODO: Anything outside of renderer should first collate commands
    // This should allow for unloaded sprites to be identified, allowing
    // renderer to load them in first before trying to render them, instead
    // of rendering them on the next frame.
    //
    // This should also help if there are many draw calls, and it also
    // should facilitate objects which use the same resources
    //
    // Best implementation of these would make the draw_X commands internal,
    // and instead provide an exposed "draw" function that instead
    // adds the draw command to a staging array.

    // TODO: Clay renderer should have an id: data unordered map to allow
    // for basic animations like css
    // Would also probably need to make a Tween class or just lerp it.

    ClayRenderer::render_commands(renderer, render_commands);
    SpriteSystem::draw_all(renderer);
    renderer.end_frame();
  }
  SDL_Log("Exiting...");
  cleanup();
  return 0;
}
