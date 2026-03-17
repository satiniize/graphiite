#pragma once
#include "../../texture.hpp"
#include "../theme.hpp"
#include "clay.h"

static const float HANDLE_SIZE = 48.0f;
static const float TRACK_WIDTH = 300.0f;
// The usable range the handle center can travel
static const float TRAVEL = TRACK_WIDTH - HANDLE_SIZE;

// userData points to a float in [0, 1]
static inline void slider_interaction(Clay_ElementId elementId,
                                      Clay_PointerData pointerInfo,
                                      intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME ||
      pointerInfo.state == CLAY_POINTER_DATA_PRESSED) {
    float *normalized = reinterpret_cast<float *>(userData);
    Clay_ElementData trackData = Clay_GetElementData(elementId);
    if (trackData.found) {
      float localX = pointerInfo.position.x - trackData.boundingBox.x;
      // Offset by half handle so value=0 means handle flush left,
      // value=1 means handle flush right
      float raw = (localX - HANDLE_SIZE / 2.0f) / TRAVEL;
      // Clamp to [0, 1]
      if (raw < 0.0f)
        raw = 0.0f;
      if (raw > 1.0f)
        raw = 1.0f;
      *normalized = raw;
    }
  }
}

// TODO: Pass in pointer state so we can move the slider even if the mouse isn't
// hovering on the element value: pointer to float in [0, 1]
static inline void Slider(float *value) {
  CLAY({
      // .id = CLAY_IDI("SliderTrack", id),
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_FIXED(TRACK_WIDTH),
                      .height = CLAY_SIZING_FIXED(HANDLE_SIZE),
                  },
          },
      .backgroundColor = Color::WHITE,
  }) {
    Clay_OnHover(slider_interaction, reinterpret_cast<intptr_t>(value));

    // Pixel offset of the handle's left edge
    float offset = (*value) * TRAVEL;

    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_FIXED(HANDLE_SIZE),
                        .height = CLAY_SIZING_FIXED(HANDLE_SIZE),
                    },
                .padding = {.left = 2, .right = 2, .top = 2, .bottom = 2},
                .childAlignment =
                    {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    },
            },
        .backgroundColor = Color::BLACK,
        .cornerRadius = CLAY_CORNER_RADIUS(24),
        .floating =
            {
                .offset = Clay_Vector2{offset, 0.0f},
                .attachPoints =
                    {
                        .element = CLAY_ATTACH_POINT_LEFT_CENTER,
                        .parent = CLAY_ATTACH_POINT_LEFT_CENTER,
                    },
                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                .attachTo = CLAY_ATTACH_TO_PARENT,
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
          .backgroundColor = Clay_Hovered() ? Color::WHITE : Color::LIGHT_GREY,
          .cornerRadius = CLAY_CORNER_RADIUS(22),
          .border =
              {
                  .color = Color::WHITE,
                  .width = CLAY_BORDER_ALL(2),
              },
      }) {}
    }
  }
}
