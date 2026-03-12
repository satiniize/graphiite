#pragma once
#include "../../clay.h"
#include "../../texture.hpp"

#include "../theme.hpp"

static inline void Placeholder(Texture test_texture) {
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_GROW(),
                      .height = CLAY_SIZING_GROW(),
                  },
              .childAlignment =
                  {
                      .x = CLAY_ALIGN_X_CENTER,
                      .y = CLAY_ALIGN_Y_CENTER,
                  },
          },
      .backgroundColor = Color::PURE_WHITE,
      .image =
          {
              .imageData = static_cast<void *>(&test_texture),
          },
  }) {
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width = CLAY_SIZING_FIT(),
                        .height = CLAY_SIZING_FIT(),
                    },
            },
    }) {
      CLAY_TEXT(CLAY_STRING("Looks like you haven't opened a folder yet."),
                CLAY_TEXT_CONFIG({
                    .textColor = Color::PURE_WHITE,
                    .fontSize = FontSize::MEDIUM,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                }));
    }
  }
}
