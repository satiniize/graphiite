#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <sys/types.h>
#include <vector>

#include "config.hpp"
#include "renderer.hpp"
#include "vec2.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Entities
#include "component_storage.hpp"
#include "entity_manager.hpp"

// Systems
// #include "physics_system.hpp"
#include "sprite_system.hpp"

// Components
#include "rigidbody_component.hpp"
// #include "sprite_component.hpp"
#include "transform_component.hpp"

Renderer renderer;

bool init() {
  // For every sprite, add their paths to an array
  // Send this array to renderer to load data and transfer to gpu
  renderer.init();
  return true;
}

bool loop() { return true; }

bool cleanup() {
  renderer.cleanup();
  return true;
}

int main(int argc, char *argv[]) {
  bool success = init();

  if (!success) {
    return success;
  }

  EntityManager entity_manager;

  // Cursor
  EntityID cursor = entity_manager.create();
  SpriteComponent cursor_sprite = {
      .path = "res/uv.bmp",
  };
  sprite_components[cursor] = cursor_sprite;
  TransformComponent cursor_transform = {
      .scale = glm::vec2(16.0f, 16.0f),
  };
  transform_components[cursor] = cursor_transform;

  // Player Amogus
  EntityID amogus = entity_manager.create();
  SpriteComponent sprite_component = {
      .path = "res/amogus.bmp",
  };
  sprite_components[amogus] = sprite_component;
  TransformComponent transform_component = {
      .position = glm::vec2(64.0f, 64.0f),
      .scale = glm::vec2(16.0f, 16.0f),
  };
  transform_components[amogus] = transform_component;

  std::mt19937 random_engine;
  std::uniform_real_distribution<float> velocity_distribution(-1.0f, 1.0f);
  std::uniform_real_distribution<float> position_distribution(0.0f, 1.0f);
  float velocity_magnitude = 512.0f;
  int friends = 16;
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

  std::vector<std::string> image_paths = {};
  for (auto &[entity_id, sprite_component] : sprite_components) {
    image_paths.push_back(sprite_component.path);
  }
  // renderer.load_textures(image_paths);
  renderer.load_texture("res/test.png");
  renderer.load_texture("res/amogus.bmp");
  renderer.load_texture("res/uv.bmp");

  uint32_t prev_frame_tick = SDL_GetTicks();
  float physics_delta_time = 1.0f / physics_tick_rate;
  float process_delta_time = 0.0f;
  float accumulator = 0.0;

  int physics_frame_count = 0;
  int process_frame_count = 0;
  float time_scale = 1.0f;

  bool running = true;

  float rotation = 0.0;

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

      // Get entity transform and rigidbody
      TransformComponent &amogus_transform = transform_components[amogus];
      RigidBodyComponent &amogus_rigidbody = rigidbody_components[amogus];

      // Player specific code
      // WASD Movement
      float accel = 512.0f;
      vec2 move_dir = vec2(0.0f, 0.0f);
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
      amogus_rigidbody.velocity += move_dir * accel * physics_delta_time;

      // Showcase rotation
      amogus_transform.rotation += 2.0f * physics_delta_time;

      // Physics tick-rate ECS system
      // PhysicsSystem::Update(physics_delta_time);
    }

    renderer.begin_frame();
    SpriteSystem::draw_all(renderer);
    rotation += physics_delta_time;

    // renderer.draw_sprite("res/test.png", glm::vec2(96.0f, 96.0f), rotation,
    // glm::vec2(128.0f, 128.0f));

    renderer.end_frame();

    loop();
  }
  SDL_Log("Exiting...");
  cleanup();
  return 0;
}
