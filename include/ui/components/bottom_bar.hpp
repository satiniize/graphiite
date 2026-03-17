#pragma once
#include "../../texture.hpp"
#include "clay.h"

#include "../theme.hpp"

#include "button.hpp"
#include "tally.hpp"

static inline void
BottomBar(Texture &edge_sheen_data, Texture &bg_sheen_data,
          std::string tally_label,
          void on_open_folder(Clay_ElementId elementId,
                              Clay_PointerData pointerInfo, intptr_t userData),
          void on_finalize(Clay_ElementId elementId,
                           Clay_PointerData pointerInfo, intptr_t userData),
          void on_sort(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                       intptr_t userData),
          void on_filter(Clay_ElementId elementId, Clay_PointerData pointerInfo,
                         intptr_t userData),
          intptr_t userData) {
  float bottom_bar_corner_radius = 36.0f;
  CLAY({
      .id = CLAY_ID("BottomBar"),
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_GROW(0),
                      .height = CLAY_SIZING_FIT(0),
                  },
              .padding =
                  {
                      .left = 4,
                      .right = 4,
                      .top = 4,
                      .bottom = 4,
                  },
          },
      .backgroundColor = Color::BLACK,
      .cornerRadius =
          {
              .topLeft = bottom_bar_corner_radius,
              .topRight = bottom_bar_corner_radius,
              .bottomLeft = 0,
              .bottomRight = 0,
          },
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .floating =
          {
              .attachPoints =
                  {
                      .element = CLAY_ATTACH_POINT_CENTER_BOTTOM,
                      .parent = CLAY_ATTACH_POINT_CENTER_BOTTOM,
                  },
              .attachTo = CLAY_ATTACH_TO_PARENT,
          },
      .border =
          {
              .color = Color::WHITE,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
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
                .padding =
                    {
                        .left = 4,
                        .right = 4,
                        .top = 4,
                        .bottom = 4,
                    },
            },
        .backgroundColor = Color::ORANGE,
        .cornerRadius =
            {
                .topLeft = bottom_bar_corner_radius - 4,
                .topRight = bottom_bar_corner_radius - 4,
                .bottomLeft = 0,
                .bottomRight = 0,
            },
    }) {
      // Left Align
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_LEFT,
                          .y = CLAY_ALIGN_Y_CENTER,
                      },
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              },
      }) {
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Open Folder"),
               on_open_folder, userData);
        CLAY({
            .layout =
                {
                    .sizing =
                        {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_GROW(0),
                        },
                },
        }) {}
        Tally(edge_sheen_data, bg_sheen_data,
              Clay_String{
                  .length = static_cast<int32_t>(tally_label.length()),
                  .chars = tally_label.c_str(),
              });
      }
      // Center Align
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_FIT(),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  .padding =
                      {
                          .left = 4,
                          .right = 4,
                      },
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_CENTER,
                      },
              },
      }) {
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Finalize"),
               on_finalize);
      }
      // Right Align
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_GROW(0),
                          .height = CLAY_SIZING_GROW(0),
                      },
                  .childGap = 4,
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_RIGHT,
                      },
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              },
      }) {
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Sort"), on_sort);
        Button(edge_sheen_data, bg_sheen_data, CLAY_STRING("Filters"),
               on_filter);
      }
    }
  }
}
