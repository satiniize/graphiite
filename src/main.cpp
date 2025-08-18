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

// Entities
#include "component_storage.hpp"
#include "entity_manager.hpp"

// Systems
#include "physics_system.hpp"
// #include "rendering_system.hpp"

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

bool loop() {
  renderer.loop();
  return true;
}

bool cleanup() {
  renderer.cleanup();
  return true;
}

std::string strip_lr(const std::string &str) {
  int start = 0;
  while (start < str.length() &&
         std::isspace(static_cast<unsigned char>(str[start]))) {
    ++start;
  }
  int end = str.length() - 1;
  while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
    end--;
  }
  return str.substr(start, end - start + 1);
}

int main(int argc, char *argv[]) {
  bool success = init();

  if (!success) {
    return success;
  }

  uint32_t prev_frame_tick = SDL_GetTicks();
  float physics_delta_time = 1.0f / physics_tick_rate;
  float process_delta_time = 0.0f;
  float accumulator = 0.0;

  int physics_frame_count = 0;
  int process_frame_count = 0;
  float time_scale = 1.0f;

  bool running = true;

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
    }

    loop();
  }
  SDL_Log("Exiting...");
  cleanup();
  return 0;
}
