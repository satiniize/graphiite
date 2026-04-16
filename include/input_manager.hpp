#pragma once

#include <clay.h>

struct Inputs {};

class InputManager {
public:
  Clay_Vector2 mouse_position = {0.0f, 0.0f};
  Clay_Vector2 mouse_scroll = {0.0f, 0.0f};

  bool is_mouse_down = false;
  bool was_mouse_down = false;
  bool is_mouse_released = false;

  float scroll_speed = 2.0f;

  InputManager() {}
  void poll_events(bool *running);
};
