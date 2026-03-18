#include "input_manager.hpp"

#include <SDL3/SDL.h>

void InputManager::poll_events(bool *running) {
  this->mouse_scroll = {0.0f, 0.0f};
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      *running = false;
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      mouse_position.x = event.wheel.mouse_x;
      mouse_position.y = event.wheel.mouse_y;

      mouse_scroll.x = event.wheel.x * scroll_speed;
      mouse_scroll.y = event.wheel.y * scroll_speed;
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
}
