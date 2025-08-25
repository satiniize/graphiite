#include <cmath>
#include <cstdint>
#include <format>
#include <random>
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

uint16_t shut_up_data[1];
std::string test_image_path = "res/uv.bmp";

Renderer renderer;

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
      }){CLAY({
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
      }){dropdown_menu();
    }
  };
}
}
}

inline void PhotoItem() {
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
      .backgroundColor = COLOR_WHITE,
      .cornerRadius = CLAY_CORNER_RADIUS(3),
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
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(3),
            },
        .aspectRatio =
            {
                .aspectRatio = (3.0f / 2.0f),
            },
        .image =
            {
                .imageData = static_cast<void *>(&test_image_path),
            },
    }) {
      CLAY_TEXT(CLAY_STRING("IMGTEST.JPG"),
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 255, 255, 255},
                    .fontSize = 16,
                }));
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
          .backgroundColor = COLOR_WHITE,
          .cornerRadius = CLAY_CORNER_RADIUS(3),
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
            .backgroundColor = Clay_Hovered() ? COLOR_GREY : COLOR_DARK_GREY,
        }) {}
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

  while (running) {
    uint32_t frame_tick = SDL_GetTicks();
    process_delta_time =
        static_cast<float>(frame_tick - prev_frame_tick) / 1000.0f;
    accumulator += process_delta_time;
    prev_frame_tick = frame_tick;

    Clay_Vector2 mouse_scroll = {0.0f, 0.0f};
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        mouse_scroll.y = event.wheel.y;
      }
    }

    float cursor_x;
    float cursor_y;
    SDL_GetMouseState(&cursor_x, &cursor_y);

    Clay_Vector2 mouse_position = {cursor_x, cursor_y};
    Clay_SetPointerState(mouse_position, false);
    Clay_UpdateScrollContainers(true, mouse_scroll, process_delta_time);

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
    cursor_transform.position = glm::vec2(cursor_x, cursor_y);

    renderer.begin_frame();
    int image_minimum_width = 240 * renderer.viewport_scale;
    int photo_columns = renderer.width / image_minimum_width;
    Clay_Dimensions clay_dimensions = {
        .width = static_cast<float>(renderer.width) /
                 static_cast<float>(renderer.viewport_scale),
        .height = static_cast<float>(renderer.height) /
                  static_cast<float>(renderer.viewport_scale)};

    Clay_SetLayoutDimensions(clay_dimensions);

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
    }) {
      CLAY({
          .id = CLAY_ID("MenuBar"),
          .layout =
              {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_FIT(0)},
                  .padding =
                      {
                          .left = 0,
                          .right = 0,
                          .top = 0,
                          .bottom = 3,
                      },
              },
          .backgroundColor = COLOR_WHITE,
          .border =
              {
                  .color = COLOR_BLACK,
                  .width =
                      {
                          .left = 0,
                          .right = 0,
                          .top = 0,
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
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 4,
                },
            .backgroundColor = COLOR_DARK_GREY,
        }) {
          MenuBarButton(CLAY_STRING("Session"), SessionDropdownMenu);
          MenuBarButton(CLAY_STRING("Edit"), EditDropdownMenu);
          MenuBarButton(CLAY_STRING("View"), ViewDropdownMenu);
          MenuBarButton(CLAY_STRING("Help"), HelpDropdownMenu);
        }
      }
      CLAY({
          .id = CLAY_ID("MarginContainer1"),
          .layout =
              {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_GROW(0)},
                  .padding = CLAY_PADDING_ALL(4),
                  .childGap = 4,
                  .layoutDirection = CLAY_TOP_TO_BOTTOM,
              },
          .backgroundColor = COLOR_GREY,
          .clip =
              {
                  .vertical = true,
                  .childOffset = Clay_GetScrollOffset(),
              },
      }) {
        for (int i = 0; i < 16; i++) {
          CLAY({.layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0),
                               .height = CLAY_SIZING_FIT(0)},
                    .childGap = 4,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }}) {
            for (int i = 0; i < photo_columns; i++) {
              PhotoItem();
            }
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
