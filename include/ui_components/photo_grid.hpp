#pragma once
#include "../clay.h"
#include "../texture.hpp"

#include "../theme.hpp"
#include <vector>

#include "photo_item.hpp"

static inline void PhotoGrid(Texture &edge_sheen_data, Texture &bg_sheen_data,
                             Texture &check_data, std::vector<Photo> &photos,
                             int image_minimum_width) {
  // Photo grid calculation
  int image_counter = 0;
  // int photo_columns = renderer.width / image_minimum_width;
  int photo_columns = 1080 / image_minimum_width;

  int num_images = std::size(photos);
  CLAY({
      .id = CLAY_ID("PhotoGrid"),
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_GROW(0),
                      .height = CLAY_SIZING_GROW(0),
                  },
          },
      .clip =
          {
              .vertical = true,
              .childOffset = Clay_GetScrollOffset(), // Somehow this function
                                                     // resets the scroll to 0
                                                     // when the overlay is
                                                     // first rendered
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
                .padding = CLAY_PADDING_ALL(8),
                .childGap = 4,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
    }) {
      for (int i = 0; i < 16 && image_counter < num_images; i++) {
        CLAY({.layout = {
                  .sizing = {.width = CLAY_SIZING_GROW(0),
                             .height = CLAY_SIZING_FIT(0)},
                  .childGap = 4,
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              }}) {
          for (int i = 0; i < photo_columns; i++) {
            if (image_counter < num_images) {
              PhotoItem(edge_sheen_data, bg_sheen_data, check_data,
                        photos[image_counter]);
              image_counter++;
            } else {
              CLAY({
                  .layout =
                      {
                          .sizing = {.width = CLAY_SIZING_GROW(0),
                                     .height = CLAY_SIZING_GROW(0)},
                      },
              }) {}
            }
          }
        }
      }
    }
  }
}
