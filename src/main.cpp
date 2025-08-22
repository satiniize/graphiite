#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <sys/types.h>
#include <vector>

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include "SDL3_ttf/SDL_ttf.h"
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

const Clay_Color COLOR_BG = {24, 24, 24, 255};
const Clay_Color COLOR_FG1 = {32, 32, 32, 255};
const Clay_Color COLOR_FG2 = {44, 44, 44, 255};
const Clay_Color COLOR_BUTTON_NORMAL = {48, 48, 48, 255};
const Clay_Color COLOR_BUTTON_HOVER = {60, 60, 60, 255};

uint16_t shut_up_data[1];

Renderer renderer;

Clay_ElementDeclaration sidebarItemConfig = (Clay_ElementDeclaration){
    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_FIXED(32)}},
    .backgroundColor = COLOR_FG2};

// Re-useable components are just normal functions
void SidebarItemComponent() {
  CLAY(sidebarItemConfig) {}
}

// Clay_ElementDeclaration menubar_button_config = (Clay_ElementDeclaration)

// void MenuBarButton() {
//   CLAY(menubar_button_config) {}
// }

void handle_clay_errors(Clay_ErrorData errorData) {
  // See the Clay_ErrorData struct for more information
  printf("%s", errorData.errorText.chars);
  switch (errorData.errorType) {
    // etc
  }
}

// Example measure text function
static inline Clay_Dimensions MeasureText(Clay_StringSlice text,
                                          Clay_TextElementConfig *config,
                                          void *userData) {
  // Clay_TextElementConfig contains members such as fontId, fontSize,
  // letterSpacing etc Note: Clay_String->chars is not guaranteed to be null
  // terminated
  return (Clay_Dimensions){
      .width = (float)text.length *
               config->fontSize, // <- this will only work for monospace fonts,
                                 // see the renderers/ directory for more
                                 // advanced text measurement
      .height = (float)config->fontSize};
}

bool init() {
  // For every sprite, add their paths to an array
  // Send this array to renderer to load data and transfer to gpu
  renderer.init();

  std::vector<std::string> loaded_sprite_paths;
  for (auto &[entity_id, sprite_component] : sprite_components) {
    // TODO: Unsure if this is efficient
    auto it = std::find(loaded_sprite_paths.begin(), loaded_sprite_paths.end(),
                        sprite_component.path);
    if (it == loaded_sprite_paths.end()) {
      SDL_Surface *image_data = IMG_Load(sprite_component.path.c_str());
      if (image_data == NULL) {
        SDL_Log("Failed to load image! %s", sprite_component.path.c_str());
        return false;
      }
      renderer.load_texture(sprite_component.path, image_data);
      SDL_DestroySurface(image_data);
      loaded_sprite_paths.push_back(sprite_component.path);
    }
    // else {
    // sprite_component.texture_id = it - loaded_sprite_paths.begin();
    // }
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

  std::mt19937 random_engine;
  std::uniform_real_distribution<float> velocity_distribution(-1.0f, 1.0f);
  std::uniform_real_distribution<float> position_distribution(0.0f, 1.0f);
  int friends = 0;
  for (int i = 0; i < friends; ++i) {
    EntityID amogus2 = entity_manager.create();
    SpriteComponent sprite_component2 = {
        .path = "res/amogus.bmp",
    };
    sprite_components[amogus2] = sprite_component2;
    TransformComponent transform_component2;
    transform_component2.position =
        glm::vec2(16.0f + (static_cast<float>(WIDTH) - 32.0f) *
                              position_distribution(random_engine),
                  16.0f + (static_cast<float>(HEIGHT) - 32.0f) *
                              position_distribution(random_engine));
    transform_component2.scale = glm::vec2(16.0f, 16.0f),
    transform_components[amogus2] = transform_component2;
  }

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
  glm::vec2 velocity = glm::vec2(0.0f, 0.0f);

  uint64_t total_memory_size = Clay_MinMemorySize();
  Clay_Arena clay_memory = Clay_CreateArenaWithCapacityAndMemory(
      total_memory_size, malloc(total_memory_size));
  Clay_Dimensions clay_dimensions = {.width = (float)WIDTH,
                                     .height = (float)HEIGHT};
  Clay_Context *clayContextBottom = Clay_Initialize(
      clay_memory, clay_dimensions, (Clay_ErrorHandler){handle_clay_errors});
  shut_up_data[1] = 1;
  Clay_SetMeasureTextFunction(MeasureText, (void *)shut_up_data);

  if (!TTF_Init()) {
    SDL_Log("Failed to initialize TTF");
    SDL_Quit();
  }

  TTF_Font *font = TTF_OpenFont("./res/IBMPlexMono-Regular.ttf", 256);
  if (!font) {
    SDL_Log("Failed to load font");
    SDL_Quit();
  }

  TTF_ImageType glyph_image_type;
  SDL_Surface *test_glyph = TTF_GetGlyphImage(font, 34, &glyph_image_type);
  int test_minx, test_maxx, test_miny, test_maxy, test_advance;
  TTF_GetGlyphMetrics(font, 64, &test_minx, &test_maxx, &test_miny, &test_maxy,
                      &test_advance);
  int height = TTF_GetFontHeight(font);
  SDL_Surface *ascii_glyph_atlas =
      SDL_CreateSurface(test_advance * 10, height * 10, test_glyph->format);
  SDL_DestroySurface(test_glyph);
  std::cout << TTF_GetFontDescent(font) << std::endl;
  for (int i = 33; i <= 126; i++) {
    int x = (i - 33) % 10;
    int y = (i - 33) / 10;
    SDL_Rect bg = {x * test_advance, y * height, test_advance, height};
    SDL_FillSurfaceRect(ascii_glyph_atlas, &bg,
                        (x + y) % 2 == 0 ? 0xFF0000FF : 0xFFFF0000);
    SDL_Surface *glyph = TTF_GetGlyphImage(font, i, &glyph_image_type);
    int minx, maxx, miny, maxy, advance;
    TTF_GetGlyphMetrics(font, i, &minx, &maxx, &miny, &maxy, &advance);
    SDL_Rect dest = {x * test_advance + minx,
                     (y + 1) * height - maxy + TTF_GetFontDescent(font), 0, 0};
    SDL_BlitSurface(glyph, NULL, ascii_glyph_atlas, &dest);
    SDL_DestroySurface(glyph);
  }

  renderer.load_texture("FONT_GLYPH", ascii_glyph_atlas);

  while (running) {
    uint32_t frame_tick = SDL_GetTicks();
    process_delta_time =
        static_cast<float>(frame_tick - prev_frame_tick) / 1000.0f;
    accumulator += process_delta_time;
    prev_frame_tick = frame_tick;

    float cursor_x;
    float cursor_y;

    SDL_GetMouseState(&cursor_x, &cursor_y);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_MOUSE_WHEEL) {
      }
    }

    Clay_Vector2 mouse_position = {cursor_x / viewport_scale,
                                   cursor_y / viewport_scale};
    Clay_SetPointerState(mouse_position, false);

    const bool *keystate = SDL_GetKeyboardState(NULL);

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

    TransformComponent &cursorTransform = transform_components[amogus];
    cursorTransform.position =
        glm::vec2(cursor_x, cursor_y) / (float)viewport_scale;

    renderer.begin_frame();
    Clay_Dimensions clay_dimensions = {
        .width = (float)(renderer.width / viewport_scale),
        .height = (float)(renderer.height / viewport_scale)};

    Clay_SetLayoutDimensions(clay_dimensions);
    Clay_BeginLayout();

    Clay_LayoutConfig layoutElement = Clay_LayoutConfig{.padding = {5}};
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
                  .sizing = {.width = CLAY_SIZING_GROW(0), .height = 8},
                  .childGap = 2,
              },
          .backgroundColor = COLOR_FG2,
      }) {
        for (int i = 0; i < 5; i++) {
          CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(16),
                                      .height = CLAY_SIZING_FIXED(8)}},
                .backgroundColor = Clay_Hovered() ? COLOR_BUTTON_HOVER
                                                  : COLOR_BUTTON_NORMAL}) {}
        }
      }
      CLAY({.id = CLAY_ID("Content"),
            .layout =
                {
                    .sizing = {.width = 128, .height = CLAY_SIZING_GROW(0)},
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 4,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
            .backgroundColor = COLOR_FG1}) {
        for (int i = 0; i < 4; i++) {
          SidebarItemComponent();
        }
      }
    }
    Clay_RenderCommandArray render_commands = Clay_EndLayout();

    ClayRenderer::render_commands(renderer, render_commands);
    SpriteSystem::draw_all(renderer);
    renderer.draw_sprite("FONT_GLYPH",
                         glm::vec2(test_advance / 2.0, height / 2.0), 0.0,
                         glm::vec2(test_advance, -height));
    renderer.end_frame();
  }
  SDL_Log("Exiting...");
  cleanup();
  return 0;
}
