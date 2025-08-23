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

const Clay_Color COLOR_BG = {12, 8, 12, 255};
const Clay_Color COLOR_FG1 = {28, 18, 28, 255};
const Clay_Color COLOR_FG2 = {36, 28, 36, 255};
const Clay_Color COLOR_BUTTON_NORMAL = COLOR_FG1;
const Clay_Color COLOR_BUTTON_HOVER = COLOR_FG2;

uint16_t shut_up_data[1];

Renderer renderer;

void MenuBarButton(Clay_String label) {
  CLAY({.layout =
            {
                .sizing = {.width = CLAY_SIZING_FIT(16, 64),
                           .height = CLAY_SIZING_FIT(16, 64)},
                .padding =
                    {
                        .left = 8,
                        .right = 8,
                        .top = 4,
                        .bottom = 4,
                    },
                .childAlignment =
                    {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    },
            },
        .backgroundColor =
            Clay_Hovered() ? COLOR_BUTTON_HOVER : COLOR_BUTTON_NORMAL}) {
    CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                         .textColor = {255, 255, 255, 255},
                         .fontSize = 12,
                     }));
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
      .scale = glm::vec2(4.0f, 4.0f),
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
  shut_up_data[1] = 1;
  Clay_SetMeasureTextFunction(MeasureText, (void *)shut_up_data);

  while (running) {
    uint32_t frame_tick = SDL_GetTicks();
    process_delta_time =
        static_cast<float>(frame_tick - prev_frame_tick) / 1000.0f;
    accumulator += process_delta_time;
    prev_frame_tick = frame_tick;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_MOUSE_WHEEL) {
      }
    }

    float cursor_x;
    float cursor_y;
    SDL_GetMouseState(&cursor_x, &cursor_y);

    // Clay_Vector2 mouse_position = {cursor_x / renderer.viewport_scale,
    //                                cursor_y / renderer.viewport_scale};
    Clay_Vector2 mouse_position = {cursor_x, cursor_y};
    Clay_SetPointerState(mouse_position, false);

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
    // cursor_transform.position =
    //     glm::vec2(cursor_x, cursor_y) / (float)renderer.viewport_scale;
    cursor_transform.position = glm::vec2(cursor_x, cursor_y);

    renderer.begin_frame();
    Clay_Dimensions clay_dimensions = {
        .width = static_cast<float>(renderer.width) /
                 static_cast<float>(renderer.viewport_scale),
        .height = static_cast<float>(renderer.height) /
                  static_cast<float>(renderer.viewport_scale)};
    Clay_SetLayoutDimensions(clay_dimensions);

    Clay_BeginLayout();
    CLAY({.id = CLAY_ID("Root"),
          .layout =
              {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_GROW(0)},
                  .padding = CLAY_PADDING_ALL(0),
                  .layoutDirection = CLAY_TOP_TO_BOTTOM,
              },
          .backgroundColor = COLOR_BG}) {
      CLAY({
          .id = CLAY_ID("MenuBar"),
          .layout =
              {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_FIT(16, 64)},
                  .childGap = 2,
              },
          .backgroundColor = COLOR_FG1,
      }) {
        MenuBarButton(CLAY_STRING("File"));
        MenuBarButton(CLAY_STRING("Edit"));
        MenuBarButton(CLAY_STRING("View"));
        MenuBarButton(CLAY_STRING("Help"));
      }
      CLAY({.id = CLAY_ID("MarginContainer1"),
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(8),
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }}) {
        for (int i = 0; i < 4; i++) {
          CLAY({.layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0),
                               .height = CLAY_SIZING_GROW(0)},
                    .childGap = 8,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }}) {
            for (int i = 0; i < 4; i++) {
              CLAY({
                  .layout =
                      {
                          .sizing = {.width = CLAY_SIZING_GROW(0),
                                     .height = CLAY_SIZING_GROW(0)},
                      },
                  .backgroundColor = Clay_Hovered() ? COLOR_FG2 : COLOR_FG1,
                  .cornerRadius =
                      {
                          .topLeft = 32,
                          .topRight = 32,
                          .bottomLeft = 32,
                          .bottomRight = 32,
                      },
              }) {}
            }
          }
        }
      }
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
