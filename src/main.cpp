#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// Libraries
#include <SDL3/SDL.h>

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include "tinyfiledialogs.h"

#include "clay_renderer.hpp"
#include "renderer.hpp"

// ECS
#include "entity_manager.hpp"
// Components
#include "sprite_component.hpp"
#include "transform_component.hpp"
// Systems
#include "image_loader.hpp"
#include "sprite_renderer.hpp"

// Resources
#include "image.hpp"
#include "texture.hpp"

// UI theming
#include "ui/theme.hpp"

#include "ui/components/bottom_bar.hpp"
#include "ui/components/photo_grid.hpp"
#include "ui/components/placeholder.hpp"

#include "ui/types/photo.hpp"

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

struct PendingTexture {
  Image image;
  std::filesystem::path path;
  bool tiling = false;
};

struct AppState {
  bool folder_opened = false;
  std::string tally_label = "XX";
  std::vector<Photo> photos;

  std::queue<PendingTexture> upload_queue;
  std::mutex upload_mutex;
  std::atomic<bool> loading_done = false;
  std::thread load_thread;
};

void load_images_worker(const std::vector<std::filesystem::path> &paths,
                        AppState *app_state) {
  for (const auto &path : paths) {
    Image image = ImageLoader::load_with_turbojpeg(path);
    {
      std::lock_guard<std::mutex> lock(app_state->upload_mutex);
      app_state->upload_queue.push({std::move(image), path, false});
    }
  }
  app_state->loading_done = true;
};

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

void on_open_folder(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                    intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {

    char *lTheSelectFolderName = tinyfd_selectFolderDialog(
        "Choose a folder containing your photos", "./");

    if (!lTheSelectFolderName) {
      return;
    }

    AppState *app_state = (AppState *)userData;
    app_state->folder_opened = true;

    std::filesystem::path folder_path(lTheSelectFolderName);

    std::vector<std::filesystem::path> paths_to_load;

    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
      paths_to_load.push_back(entry);
    }

    app_state->load_thread =
        std::thread(load_images_worker, std::move(paths_to_load), app_state);
  }
}

void on_finalize(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                 intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
    // folder_opened = false;
    // photos.clear();
  }
}

void on_sort(Clay_ElementId elementId, Clay_PointerData pointerInfo,
             intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
  }
}

void on_filter(Clay_ElementId elementId, Clay_PointerData pointerInfo,
               intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
  }
}

static inline Clay_Dimensions MeasureText(Clay_StringSlice text,
                                          Clay_TextElementConfig *config,
                                          void *userData) {
  Renderer &renderer = *(Renderer *)userData;
  float scalar = config->fontSize / renderer.font_sample_point_size;
  return Clay_Dimensions{.width = (float)text.length * renderer.glyph_size.x *
                                  scalar,
                         .height = (float)renderer.glyph_size.y * scalar};
}

Texture load_and_upload_texture(Renderer &renderer, std::string path,
                                bool tiling = false) {
  Image image = ImageLoader::load(path);
  TextureID texture_id =
      renderer.load_texture(image.pixels.data(), image.width, image.height);

  Texture texture_data;
  texture_data.path = path;
  texture_data.tiling = tiling;
  texture_data.id = texture_id;

  SDL_Log("Loaded texture %s with id %zu", path.c_str(), texture_id);

  return texture_data;
}

int main(int argc, char *argv[]) {
  AppState app_state;

  // ECS
  std::unordered_map<EntityID, SpriteComponent> sprite_components;
  std::unordered_map<EntityID, TransformComponent> transform_components;

  EntityManager entity_manager;

  EntityID cursor = entity_manager.create();
  SpriteComponent sprite_component = {
      .path = "res/uv.bmp",
  };
  sprite_components[cursor] = sprite_component;
  TransformComponent transform_component = {
      .position = glm::vec2(64.0f, 64.0f),
      .scale = glm::vec2(1.0f, 1.0f),
  };
  transform_components[cursor] = transform_component;

  // Renderer
  const int WIDTH = 1080;
  const int HEIGHT = 720;
  Renderer renderer(WIDTH, HEIGHT);

  // Load sprite textures
  SpriteRenderer::upload_sprites(renderer, sprite_components);

  // Clay setup
  size_t total_memory_size = Clay_MinMemorySize();
  Clay_Arena clay_memory = Clay_CreateArenaWithCapacityAndMemory(
      total_memory_size, malloc(total_memory_size));
  Clay_Dimensions clay_dimensions = {.width = (float)WIDTH,
                                     .height = (float)HEIGHT};
  Clay_Context *clayContextBottom = Clay_Initialize(
      clay_memory, clay_dimensions, Clay_ErrorHandler{handle_clay_errors});

  Clay_SetMeasureTextFunction(MeasureText, &renderer);

  // Manually load Clay textures
  Texture edge_sheen_data =
      load_and_upload_texture(renderer, "res/edge_sheen.png");
  Texture carbon_fiber_data =
      load_and_upload_texture(renderer, "res/carbon_fiber.png", true);
  Texture vignette_data = load_and_upload_texture(renderer, "res/vignette.png");
  Texture bg_sheen_data = load_and_upload_texture(renderer, "res/bg_sheen.png");
  Texture check_data = load_and_upload_texture(renderer, "res/check.png");

  // Timing
  const int physics_tick_rate = 60;
  const float physics_delta_time = 1.0f / static_cast<float>(physics_tick_rate);

  uint32_t prev_frame_tick = SDL_GetTicks();
  float process_delta_time = 0.0f;
  float accumulator = 0.0;
  float time_scale = 1.0f;
  int physics_frame_count = 0;
  int process_frame_count = 0;

  float scroll_speed = 6.0f;
  bool is_mouse_down = false;
  Clay_Vector2 mouse_position = {0.0f, 0.0f};

  bool running = true;
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
    }
    if (physics_frame_count >= physics_tick_rate) {
      SDL_Log("FPS: %d", static_cast<int>(process_frame_count));
      physics_frame_count = 0;
      process_frame_count = 0;
    }

    {
      std::lock_guard<std::mutex> lock(app_state.upload_mutex);
      while (!app_state.upload_queue.empty()) {
        PendingTexture &pending = app_state.upload_queue.front();
        TextureID id =
            renderer.load_texture(pending.image.pixels.data(),
                                  pending.image.width, pending.image.height);
        Texture tex;
        tex.path = pending.path;
        tex.tiling = false;
        tex.id = id;

        Photo photo;
        photo.image_data = tex;
        photo.selected = false;
        photo.file_path = pending.path;

        app_state.photos.push_back(photo);
        app_state.upload_queue.pop();
      }
    }

    // Poll inputs
    // const bool *keystate = SDL_GetKeyboardState(NULL);
    Clay_Vector2 mouse_scroll = {0.0f, 0.0f};
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

    TransformComponent &cursor_transform = transform_components[cursor];
    cursor_transform.position = glm::vec2(mouse_position.x, mouse_position.y);

    // Query resolution so clay
    renderer.update_swapchain_texture();

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
        .backgroundColor = Color::PURE_WHITE,
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
          .backgroundColor = Color::PURE_WHITE,
          .image =
              {
                  .imageData = static_cast<void *>(&vignette_data),
              },
      }) {
        // Image Grid
        if (app_state.folder_opened) {
          PhotoGrid(edge_sheen_data, bg_sheen_data, check_data,
                    app_state.photos,
                    renderer.width / 240.0f * renderer.viewport_scale);
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
      BottomBar(edge_sheen_data, bg_sheen_data, app_state.tally_label,
                on_open_folder, on_finalize, on_sort, on_filter,
                reinterpret_cast<intptr_t>(&app_state));
    }

    Clay_RenderCommandArray render_commands = Clay_EndLayout();
    renderer.begin_frame(); // Start here to update window dimensions for clay
    ClayRenderer::render_commands(renderer, render_commands);
    SpriteRenderer::draw_all(renderer, sprite_components, transform_components);
    renderer.end_frame();
  }
  if (app_state.load_thread.joinable()) {
    app_state.load_thread.join();
  }
  return 0;
}
