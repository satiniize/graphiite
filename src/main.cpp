#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <sys/types.h>
#include <vector>

#include "config.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "renderer.hpp"

// Entities
#include "component_storage.hpp"
#include "entity_manager.hpp"

// Systems
#include "sprite_system.hpp"

// Components
#include "sprite_component.hpp"
#include "transform_component.hpp"

Renderer renderer;

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
      renderer.load_texture(sprite_component.path);
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
      .scale = glm::vec2(16.0f, 16.0f),
  };
  transform_components[amogus] = transform_component;

  std::mt19937 random_engine;
  std::uniform_real_distribution<float> velocity_distribution(-1.0f, 1.0f);
  std::uniform_real_distribution<float> position_distribution(0.0f, 1.0f);
  int friends = 128;
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
    }

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

      TransformComponent &amogus_transform = transform_components[amogus];

      // Player specific code
      // WASD Movement
      float accel = 512.0f;
      glm::vec2 move_dir = glm::vec2(0.0f, 0.0f);
      if (keystate[SDL_SCANCODE_W]) {
        move_dir.y -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_A]) {
        move_dir.x -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_S]) {
        move_dir.y += 1.0f;
      }
      if (keystate[SDL_SCANCODE_D]) {
        move_dir.x += 1.0f;
      }
      velocity += move_dir * accel * physics_delta_time;
      amogus_transform.position += velocity * physics_delta_time;

      // Showcase rotation
      amogus_transform.rotation += 32.0f * physics_delta_time;
    }

    renderer.begin_frame();
    SpriteSystem::draw_all(renderer);
    renderer.end_frame();

    // loop();
  }
  SDL_Log("Exiting...");
  cleanup();
  return 0;
}
