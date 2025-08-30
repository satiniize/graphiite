#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

// Libraries
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_surface.h"
#include "SDL3_image/SDL_image.h"
#include "turbojpeg.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "tinyfiledialogs.h"

#include "clay_renderer.hpp"
#include "config.hpp"
#include "renderer.hpp"

// Entities
#include "component_storage.hpp"
#include "entity_manager.hpp"

// Systems
#include "sprite_system.hpp"

// Components
#include "sprite_component.hpp"
#include "transform_component.hpp"

const Clay_Color COLOR_WHITE = {192, 192, 192, 255};
const Clay_Color COLOR_BLACK = {12, 12, 12, 255};
const Clay_Color COLOR_GREY = {96, 96, 96, 255};
const Clay_Color COLOR_DARK_GREY = {24, 24, 24, 255};
const Clay_Color COLOR_PURE_WHITE = {255, 255, 255, 255};

const Clay_Color COLOR_LIGHT_GREY = {160, 160, 160, 255};
const Clay_Color COLOR_TRANSPARENT = {255, 255, 255, 0};
const Clay_Color COLOR_SELECTED_GREEN = {127, 255, 0, 255};

uint16_t shut_up_data[1];

Renderer renderer;

ImageData edge_sheen_data;
ImageData carbon_fiber_data;
ImageData vignette_data;
ImageData bg_sheen_data;
ImageData check_data;

struct Photo {
  ImageData image_data;
  bool selected;
  std::filesystem::path file_path;
};

std::string photos_root_path = "res/FUJI/";
std::vector<Photo> photos;
bool folder_opened = false;

std::string tally_label;

// Technically very inefficient, not sure
int get_selected_photos_count() {
  int count = 0;
  for (auto photo : photos) {
    if (photo.selected) {
      count++;
    }
  }
  return count;
}

bool seperate_photos(std::vector<Photo> &photos) {
  std::filesystem::path root_path(photos[0].file_path.parent_path());

  std::filesystem::create_directories(root_path / "Curated");
  std::filesystem::create_directories(root_path / "Discarded");

  for (auto &photo : photos) {
    if (photo.selected) {
      std::filesystem::copy_file(
          photo.file_path, root_path / "Curated" / photo.file_path.filename(),
          std::filesystem::copy_options::overwrite_existing);
    } else {
      std::filesystem::copy_file(
          photo.file_path, root_path / "Discarded" / photo.file_path.filename(),
          std::filesystem::copy_options::overwrite_existing);
    }
    std::filesystem::remove(photo.file_path);
  }
  return true;
}

bool load_photos(std::filesystem::path path) {
  if (!std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
    SDL_Log("Invalid photo path");
    return 1;
  }

  photos.clear();

  for (const std::filesystem::directory_entry &entry :
       std::filesystem::directory_iterator(path)) {
    if (entry.is_regular_file()) {
      SDL_Log("Loading file: %s", entry.path().c_str());
      // std::cout << "Loading file: " << entry.path() << std::endl;

      ImageData photo_image_data{};
      photo_image_data.path = entry.path().string();
      photo_image_data.tiling = false;

      Photo photo{};
      photo.image_data = photo_image_data;
      photo.selected = false;
      photo.file_path = entry.path();

      photos.push_back(photo);

      std::ifstream jpegStream(entry.path());
      if (!jpegStream.is_open()) {
        SDL_Log("ERROR: opening input file %s: %s", entry.path().c_str(),
                strerror(errno));
      }

      jpegStream.seekg(0, std::ios::end);
      std::streampos size = jpegStream.tellg();
      if (size == 0) {
        SDL_Log("WARNING: Input file contains no data");
      }
      jpegStream.seekg(0, std::ios::beg);
      size_t jpegSize = static_cast<size_t>(size);

      unsigned char *jpegBuf = (unsigned char *)malloc(jpegSize);
      if (!jpegBuf) {
        SDL_Log("ERROR: allocating JPEG buffer");
      }

      jpegStream.read(reinterpret_cast<char *>(jpegBuf), jpegSize);
      jpegStream.close();

      // 2. Initialize TurboJPEG decompressor
      tjhandle tjInstance = tj3Init(TJINIT_DECOMPRESS);
      if (!tjInstance) {
        SDL_Log("ERROR: creating TurboJPEG instance");
      }

      // 3. Read JPEG header to get image info
      int jpegWidth, jpegHeight, jpegSubsamp, jpegColorspace, jpegPrecision;
      if (tj3DecompressHeader(tjInstance, jpegBuf, jpegSize) < 0) {
        SDL_Log("ERROR: reading JPEG header for %s: %s", entry.path().c_str(),
                tj3GetErrorStr(tjInstance));
      }

      jpegWidth = tj3Get(tjInstance, TJPARAM_JPEGWIDTH);
      jpegHeight = tj3Get(tjInstance, TJPARAM_JPEGHEIGHT);
      jpegSubsamp = tj3Get(tjInstance, TJPARAM_SUBSAMP);
      jpegColorspace = tj3Get(tjInstance, TJPARAM_COLORSPACE);
      jpegPrecision = tj3Get(tjInstance, TJPARAM_PRECISION);

      // --- Prepare for Decompression and SDL Surface Creation ---
      // We'll target 8-bit per channel output for SDL_Surface compatibility.
      // If the JPEG is higher precision, we'll convert it.

      // For general compatibility, decompres to BGRX (32-bit per pixel)
      // For some reason, the backwards thing here happens again
      int tjDecompressFormat = TJPF_RGBA;
      SDL_PixelFormat sdlPixelFormat = SDL_PIXELFORMAT_ABGR8888;
      int sdlPixelSize =
          tjPixelSize[tjDecompressFormat]; // Will be 4 bytes for TJPF_BGRX

      // Check for higher precision JPEGs
      unsigned char *decompressedBuf_8bit =
          NULL; // This will hold the final 8-bit data for SDL
      void *tjOutputRawBuf = NULL; // Intermediate buffer if precision > 8
      int tjOutputRawBufSize = 0;

      // 8 Bit
      if (jpegPrecision <= 8) {
        tjOutputRawBufSize =
            jpegWidth * jpegHeight * sdlPixelSize; // Size for 8-bit data
        tjOutputRawBuf = malloc(tjOutputRawBufSize);
        if (!tjOutputRawBuf) {
          SDL_Log("ERROR: allocating 8-bit TurboJPEG output buffer for %s: %s",
                  entry.path().c_str(), strerror(errno));
          // goto cleanup_loop;
        }
        if (tj3Decompress8(tjInstance, jpegBuf, jpegSize,
                           (unsigned char *)tjOutputRawBuf, 0,
                           tjDecompressFormat) < 0) {
          SDL_Log("ERROR: decompressing 8-bit JPEG image %s: %s",
                  entry.path().c_str(), tj3GetErrorStr(tjInstance));
          free(tjOutputRawBuf); // Decompression failed, free buffer
          // goto cleanup_loop;
        }
        decompressedBuf_8bit =
            (unsigned char *)tjOutputRawBuf; // Direct use for 8-bit
        // SDL will take ownership of decompressedBuf_8bit later, so don't free
        // it here
        tjOutputRawBuf =
            NULL; // Clear pointer to prevent double free via cleanup_loop
      } else {    // Handle 12 or 16-bit JPEGs, convert to 8-bit for SDL
        SDL_Log("WARNING: JPEG %s has precision %d > 8 bits. Converting to "
                "8-bit per channel.",
                entry.path().c_str(), jpegPrecision);

        // TurboJPEG outputs unsigned short for precision > 8
        int tjIntermediatePixelSize_16bit =
            tjPixelSize[tjDecompressFormat] *
            sizeof(unsigned short); // e.g., 4 channels * 2 bytes/channel = 8
                                    // bytes/pixel
        tjOutputRawBufSize =
            jpegWidth * jpegHeight * tjIntermediatePixelSize_16bit;
        tjOutputRawBuf = malloc(tjOutputRawBufSize);
        if (!tjOutputRawBuf) {
          SDL_Log("ERROR: allocating 16-bit TurboJPEG intermediate buffer for "
                  "%s: %s",
                  entry.path().c_str(), strerror(errno));
          // goto cleanup_loop;
        }

        // 12 Bit
        if (jpegPrecision <= 12) {
          if (tj3Decompress12(tjInstance, jpegBuf, jpegSize,
                              (short int *)tjOutputRawBuf, 0,
                              tjDecompressFormat) < 0) {
            SDL_Log("ERROR: decompressing 12-bit JPEG image %s: %s",
                    entry.path().c_str(), tj3GetErrorStr(tjInstance));
            free(tjOutputRawBuf);
            // goto cleanup_loop;
          }
          // 12 Bit
        } else { // Assume precision <= 16
          if (tj3Decompress16(tjInstance, jpegBuf, jpegSize,
                              (unsigned short *)tjOutputRawBuf, 0,
                              tjDecompressFormat) < 0) {
            SDL_Log("ERROR: decompressing 16-bit JPEG image %s: %s",
                    entry.path().c_str(), tj3GetErrorStr(tjInstance));
            free(tjOutputRawBuf);
            // goto cleanup_loop;
          }
        }

        // Now, convert the 16-bit (unsigned short) tjOutputRawBuf to 8-bit
        // (unsigned char) decompressedBuf_8bit
        decompressedBuf_8bit = (unsigned char *)malloc(
            jpegWidth * jpegHeight * sdlPixelSize); // Final 8-bit size
        if (!decompressedBuf_8bit) {
          SDL_Log("ERROR: allocating 8-bit conversion buffer for %s: %s",
                  entry.path().c_str(), strerror(errno));
          free(tjOutputRawBuf);
          // goto cleanup_loop;
        }

        unsigned short *src_ptr = (unsigned short *)tjOutputRawBuf;
        unsigned char *dst_ptr = decompressedBuf_8bit;

        // Loop through pixels and convert
        for (int i = 0; i < jpegWidth * jpegHeight; ++i) {
          // Assuming TJPF_BGRX output (4 channels per pixel)
          *dst_ptr++ = (unsigned char)(*src_ptr++ >> 8); // B
          *dst_ptr++ = (unsigned char)(*src_ptr++ >> 8); // G
          *dst_ptr++ = (unsigned char)(*src_ptr++ >> 8); // R
          *dst_ptr++ = (unsigned char)0xFF; // X/Alpha (fully opaque)
        }
        free(tjOutputRawBuf); // Free the intermediate 16-bit buffer
        tjOutputRawBuf = NULL;
      }

      // 4. Create an SDL_Surface from the decompressed 8-bit data
      // Using the new SDL3 function: SDL_CreateSurfaceFrom
      SDL_Surface *original_image_surface = SDL_CreateSurfaceFrom(
          jpegWidth,      // Width
          jpegHeight,     // Height
          sdlPixelFormat, // SDL Pixel Format (e.g., SDL_PIXELFORMAT_BGRX8888)
          decompressedBuf_8bit,    // Pointer to existing pixel data (8-bit)
          jpegWidth * sdlPixelSize // Pitch (bytes per row)
      );

      // Important: According to SDL3 wiki for SDL_CreateSurfaceFrom,
      // "Pixel data is not managed automatically; you must free the surface
      // before you free the pixel data."
      // This means SDL *does NOT* take ownership of decompressedBuf_8bit,
      // and we MUST free it ourselves *after* Destroying
      // original_image_surface. This is a crucial change from SDL2's
      // SDL_CreateRGBSurfaceWithFormatFrom!

      if (original_image_surface == NULL) {
        SDL_Log("ERROR: Failed to create SDL_Surface from decompressed data "
                "for %s: %s",
                entry.path().c_str(), SDL_GetError());
        free(decompressedBuf_8bit); // SDL didn't take ownership, so we must
                                    // free.
        // goto cleanup_loop;
      }

      // 5. Downsample the image
      int downsample_factor = 10;
      int thumbWidth = original_image_surface->w / downsample_factor;
      int thumbHeight = original_image_surface->h / downsample_factor;
      if (thumbWidth == 0)
        thumbWidth = 1; // Ensure minimum 1 pixel
      if (thumbHeight == 0)
        thumbHeight = 1;

      SDL_Surface *downsampled = SDL_CreateSurface(
          thumbWidth, thumbHeight,
          original_image_surface
              ->format); // SDL_CreateSurface (new in SDL3 for simpler creation)

      if (downsampled == NULL) {
        SDL_Log("ERROR: Failed to create downsampled SDL_Surface for %s: %s",
                entry.path().c_str(), SDL_GetError());
        SDL_DestroySurface(original_image_surface);
        free(decompressedBuf_8bit); // Must free this explicitly!
        // goto cleanup_loop;
      }

      SDL_Rect src_rect = {0, 0, original_image_surface->w,
                           original_image_surface->h};
      SDL_Rect dst_rect = {0, 0, thumbWidth, thumbHeight};

      if (!SDL_BlitSurfaceScaled(original_image_surface, &src_rect, downsampled,
                                 &dst_rect, SDL_SCALEMODE_LINEAR)) {
        SDL_Log("ERROR: Failed to scale surface for %s: %s",
                entry.path().c_str(), SDL_GetError());
        SDL_DestroySurface(original_image_surface);
        free(decompressedBuf_8bit); // Must free this explicitly!
        SDL_DestroySurface(downsampled);
        // goto cleanup_loop;
      }

      // 6. Load texture into your renderer
      renderer.load_texture(entry.path(), downsampled);

      // 7. Clean up surfaces and TurboJPEG instance
      SDL_DestroySurface(original_image_surface);
      free(decompressedBuf_8bit); // CRITICAL: Free the buffer that
                                  // original_image_surface pointed to
      SDL_DestroySurface(downsampled);
      free(jpegBuf);
      tj3Destroy(tjInstance);
    }
  }

  return true;
}

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

void handle_photo_item_interaction(Clay_ElementId elementId,
                                   Clay_PointerData pointerInfo,
                                   intptr_t userData) {
  Photo *photo = (Photo *)userData;
  // Pointer state allows you to detect mouse down / hold / release
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    photo->selected = !photo->selected;
    // Do some click handling
    // NavigateTo(buttonData->link);
  }
}

void handle_finalize_button_interaction(Clay_ElementId elementId,
                                        Clay_PointerData pointerInfo,
                                        intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    seperate_photos(photos);
  }
}

void handle_open_folder_button_interaction(Clay_ElementId elementId,
                                           Clay_PointerData pointerInfo,
                                           intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {

    char *lTheSelectFolderName =
        tinyfd_selectFolderDialog("let us just select a directory", "../../");

    if (!lTheSelectFolderName) {
      tinyfd_messageBox("Error", "Select folder name is NULL", "ok", "error",
                        1);
      return;
    }

    tinyfd_messageBox("The selected folder is", lTheSelectFolderName, "ok",
                      "info", 1);
    folder_opened = true;

    std::filesystem::path folder_path(lTheSelectFolderName);

    load_photos(folder_path);
  }
}

void handle_sort_button_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
  }
}

void handle_filters_button_interaction(Clay_ElementId elementId,
                                       Clay_PointerData pointerInfo,
                                       intptr_t userData) {
  if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    // seperate_photos(photos);
  }
}

// TODO: Change hover to full photo rect, current selection is too small
inline void PhotoItem(Photo &photo) {
  uint16_t corner_radius = 16;
  uint16_t checkbox_corner_radius = 5;
  CLAY({
      .layout =
          {
              .sizing = {.width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_GROW(0)},
              .padding = CLAY_PADDING_ALL(3),
              .childGap = 8,
              .childAlignment =
                  {
                      .x = CLAY_ALIGN_X_CENTER,
                      .y = CLAY_ALIGN_Y_TOP,
                  },
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
          },
      .backgroundColor =
          photo.selected ? COLOR_SELECTED_GREEN : COLOR_PURE_WHITE,
      .cornerRadius = CLAY_CORNER_RADIUS(static_cast<float>(corner_radius)),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = COLOR_BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
          },
  }) {
    Clay_OnHover(handle_photo_item_interaction, (intptr_t)&photo);
    CLAY({
        .layout =
            {
                .sizing = {.width = CLAY_SIZING_GROW(0),
                           .height = CLAY_SIZING_GROW(0)},
                .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(
                    corner_radius - 3 - checkbox_corner_radius)),
            },
        .backgroundColor = COLOR_PURE_WHITE,
        .cornerRadius =
            CLAY_CORNER_RADIUS(static_cast<float>(corner_radius - 3)),
        .aspectRatio =
            {
                .aspectRatio = (3.0f / 2.0f),
            },
        .image =
            {
                .imageData = static_cast<void *>(&photo.image_data),
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
      }) {}
      CLAY({
          .layout =
              {
                  .sizing =
                      {
                          .width = CLAY_SIZING_FIXED(32),
                          .height = CLAY_SIZING_FIXED(32),
                      },
                  .padding = CLAY_PADDING_ALL(3),
              },
          .backgroundColor =
              photo.selected ? COLOR_SELECTED_GREEN : COLOR_PURE_WHITE,
          .cornerRadius =
              CLAY_CORNER_RADIUS(static_cast<float>(checkbox_corner_radius)),
          .image =
              {
                  .imageData = static_cast<void *>(&edge_sheen_data),
              },
          .border =
              {
                  .color = COLOR_BLACK,
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
                },
            .backgroundColor = COLOR_PURE_WHITE,
            .cornerRadius = CLAY_CORNER_RADIUS(
                static_cast<float>(checkbox_corner_radius - 3)),
            .image =
                {
                    .imageData = static_cast<void *>(&bg_sheen_data),
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
              .backgroundColor =
                  photo.selected ? COLOR_SELECTED_GREEN : COLOR_TRANSPARENT,
              .image =
                  {
                      .imageData = static_cast<void *>(&check_data),
                  },
          }) {}
        }
      }
    }
  }
}

void PhotoGrid(std::vector<Photo> &photos, int image_minimum_width) {
  // Photo grid calculation
  int image_counter = 0;
  int photo_columns = renderer.width / image_minimum_width;

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
              PhotoItem(photos[image_counter]);
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

void Placeholder() {
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
                    .textColor = COLOR_PURE_WHITE,
                    .fontSize = 24,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                }));
    }
  }
}

void Button(Clay_String label,
            void button_interaction(Clay_ElementId elementId,
                                    Clay_PointerData pointerInfo,
                                    intptr_t userData)) {
  uint16_t button_height = 48;
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_FIT(),
                      .height =
                          CLAY_SIZING_FIXED(static_cast<float>(button_height)),
                  },
              .padding = CLAY_PADDING_ALL(3),
          },
      .backgroundColor = COLOR_PURE_WHITE,
      .cornerRadius = CLAY_CORNER_RADIUS(button_height / 2.0f),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = COLOR_BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 2,
                  },
          },
  }) {
    Clay_OnHover(button_interaction, NULL);
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
                        .left = 16,
                        .right = 16,
                        .top = 0,
                        .bottom = 0,
                    },
                .childAlignment =
                    {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    },
            },
        .backgroundColor = Clay_Hovered() ? COLOR_PURE_WHITE : COLOR_LIGHT_GREY,
        .cornerRadius = CLAY_CORNER_RADIUS(button_height / 2.0f - 3.0f),
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
            },
    }) {
      CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                           .textColor = {255, 255, 255, 255},
                           .fontSize = 20,
                           .wrapMode = CLAY_TEXT_WRAP_NONE,
                           .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                       }));
    }
  }
}

void Tally(Clay_String label) {
  uint16_t diameter = 48;
  CLAY({
      .layout =
          {
              .sizing =
                  {
                      .width = CLAY_SIZING_FIXED(static_cast<float>(diameter)),
                      .height = CLAY_SIZING_FIXED(static_cast<float>(diameter)),
                  },
              .padding = CLAY_PADDING_ALL(3),
          },
      .backgroundColor = COLOR_SELECTED_GREEN,
      .cornerRadius = CLAY_CORNER_RADIUS(diameter / 2.0f),
      .image =
          {
              .imageData = static_cast<void *>(&edge_sheen_data),
          },
      .border =
          {
              .color = COLOR_BLACK,
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
                        .left = 0,
                        .right = 0,
                        .top = 0,
                        .bottom = 0,
                    },
                .childAlignment =
                    {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    },
            },
        .backgroundColor = Clay_Hovered() ? COLOR_PURE_WHITE : COLOR_LIGHT_GREY,
        .cornerRadius = CLAY_CORNER_RADIUS(diameter / 2.0f - 3.0f),
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
            },
    }) {
      CLAY_TEXT(label, CLAY_TEXT_CONFIG({
                           .textColor = COLOR_SELECTED_GREEN,
                           .fontSize = 20,
                           .wrapMode = CLAY_TEXT_WRAP_NONE,
                           .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                       }));
    }
  }
}

void BottomBar() {
  float bottom_bar_corner_radius = 38.0f;
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
                      .left = 3,
                      .right = 3,
                      .top = 3,
                      .bottom = 0,
                  },
          },
      .backgroundColor = COLOR_PURE_WHITE,
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
              .color = COLOR_BLACK,
              .width =
                  {
                      .left = 2,
                      .right = 2,
                      .top = 2,
                      .bottom = 0,
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
        .backgroundColor = COLOR_GREY,
        .cornerRadius =
            {
                .topLeft = bottom_bar_corner_radius - 3,
                .topRight = bottom_bar_corner_radius - 3,
                .bottomLeft = 0,
                .bottomRight = 0,
            },
        .image =
            {
                .imageData = static_cast<void *>(&bg_sheen_data),
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
        Button(CLAY_STRING("Open Folder"),
               handle_open_folder_button_interaction);
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
        Tally((Clay_String){
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
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_CENTER,
                      },
              },
      }) {
        Button(CLAY_STRING("Finalize"), handle_finalize_button_interaction);
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
                  .childAlignment =
                      {
                          .x = CLAY_ALIGN_X_RIGHT,
                      },
                  .layoutDirection = CLAY_LEFT_TO_RIGHT,
              },
      }) {
        Button(CLAY_STRING("Sort"), handle_sort_button_interaction);
        Button(CLAY_STRING("Filters"), handle_filters_button_interaction);
      }
    }
  }
}

static inline Clay_Dimensions MeasureText(Clay_StringSlice text,
                                          Clay_TextElementConfig *config,
                                          void *userData) {
  float scalar = config->fontSize / renderer.font_sample_point_size;
  return (Clay_Dimensions){.width = (float)text.length * renderer.glyph_size.x *
                                    scalar,
                           .height = (float)renderer.glyph_size.y * scalar};
}

bool init() {
  renderer.init();

  std::vector<std::string> loaded_sprite_paths;
  for (auto &[entity_id, sprite_component] : sprite_components) {
    auto it = std::find(loaded_sprite_paths.begin(), loaded_sprite_paths.end(),
                        sprite_component.path);

    if (it != loaded_sprite_paths.end()) {
      continue;
    }

    SDL_Surface *image_data = IMG_Load(sprite_component.path.c_str());
    if (!image_data) {
      SDL_Log("Failed to load image! %s", sprite_component.path.c_str());
      return false;
    }

    sprite_component.size = glm::ivec2(image_data->w, image_data->h);
    renderer.load_texture(sprite_component.path, image_data);
    SDL_DestroySurface(image_data);
    loaded_sprite_paths.push_back(sprite_component.path);
  }
  return true;
}

bool loop() { return true; }

bool cleanup() {
  renderer.cleanup();
  return true;
}

// TODO: Give feedback after submitting finalize button
// TODO: Reset screen when finalize is pressed

int main(int argc, char *argv[]) {
  char lBuffer[1024];

  tinyfd_beep();
  char *lWillBeGraphicMode = tinyfd_inputBox("tinyfd_query", NULL, NULL);

  strcpy(lBuffer, "tinyfiledialogs\nv");
  strcat(lBuffer, tinyfd_version);
  if (lWillBeGraphicMode) {
    strcat(lBuffer, "\ngraphic mode: ");
  } else {
    strcat(lBuffer, "\nconsole mode: ");
  }
  strcat(lBuffer, tinyfd_response);

  // ECS
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
      .scale = glm::vec2(1.0f, 1.0f),
  };
  transform_components[amogus] = transform_component;

  // Init(texture uploading) must be after entities are created
  if (!init()) {
    return 1;
  }

  uint64_t total_memory_size = Clay_MinMemorySize();
  Clay_Arena clay_memory = Clay_CreateArenaWithCapacityAndMemory(
      total_memory_size, malloc(total_memory_size));
  Clay_Dimensions clay_dimensions = {.width = (float)WIDTH,
                                     .height = (float)HEIGHT};
  Clay_Context *clayContextBottom = Clay_Initialize(
      clay_memory, clay_dimensions, (Clay_ErrorHandler){handle_clay_errors});
  shut_up_data[0] = 1;
  Clay_SetMeasureTextFunction(MeasureText, (void *)shut_up_data);

  SDL_Surface *edge_sheen = IMG_Load("res/edge_sheen.png");
  renderer.load_texture("res/edge_sheen.png", edge_sheen);
  SDL_DestroySurface(edge_sheen);

  edge_sheen_data.path = "res/edge_sheen.png";
  edge_sheen_data.tiling = false;

  SDL_Surface *carbon_fiber = IMG_Load("res/carbon_fiber.png");
  renderer.load_texture("res/carbon_fiber.png", carbon_fiber);
  SDL_DestroySurface(carbon_fiber);

  carbon_fiber_data.path = "res/carbon_fiber.png";
  carbon_fiber_data.tiling = true;

  SDL_Surface *vignette = IMG_Load("res/vignette.png");
  renderer.load_texture("res/vignette.png", vignette);
  SDL_DestroySurface(vignette);

  vignette_data.path = "res/vignette.png";
  vignette_data.tiling = false;

  SDL_Surface *bg_sheen = IMG_Load("res/bg_sheen.png");
  renderer.load_texture("res/bg_sheen.png", bg_sheen);
  SDL_DestroySurface(bg_sheen);

  bg_sheen_data.path = "res/bg_sheen.png";
  bg_sheen_data.tiling = false;

  SDL_Surface *check = IMG_Load("res/check.png");
  renderer.load_texture("res/check.png", check);
  SDL_DestroySurface(check);

  check_data.path = "res/check.png";
  check_data.tiling = false;

  // Timing
  uint32_t prev_frame_tick = SDL_GetTicks();
  float physics_delta_time = 1.0f / physics_tick_rate;
  float process_delta_time = 0.0f;
  float accumulator = 0.0;

  int physics_frame_count = 0;
  int process_frame_count = 0;

  float time_scale = 1.0f;

  float scroll_speed = 6.0f;
  bool is_mouse_down = false;

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
      if (physics_frame_count >= static_cast<int>(physics_tick_rate)) {
        SDL_Log("FPS: %d", static_cast<int>(process_frame_count));
        physics_frame_count = 0;
        process_frame_count = 0;
      }
    }

    // Poll inputs
    Clay_Vector2 mouse_position;
    Clay_Vector2 mouse_scroll = {0.0f, 0.0f};

    // const bool *keystate = SDL_GetKeyboardState(NULL);
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

    TransformComponent &cursor_transform = transform_components[amogus];
    cursor_transform.position = glm::vec2(mouse_position.x, mouse_position.y);

    // Get tally count
    // This is technically a bit redundant
    // but is guaranteed to not be out of sync
    std::stringstream ss;
    ss << get_selected_photos_count();
    tally_label = ss.str();

    renderer.begin_frame(); // Start here to update window dimensions for clay

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
        .backgroundColor = COLOR_PURE_WHITE,
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
          .backgroundColor = COLOR_PURE_WHITE,
          .image =
              {
                  .imageData = static_cast<void *>(&vignette_data),
              },
      }) {
        // Image Grid
        if (folder_opened) {
          PhotoGrid(photos, 240 * renderer.viewport_scale);
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
      BottomBar();
    }
    Clay_RenderCommandArray render_commands = Clay_EndLayout();

    // TODO: Anything outside of renderer should first collate commands
    // This should allow for unloaded sprites to be identified, allowing
    // renderer to load them in first before trying to render them, instead
    // of rendering them on the next frame.
    //
    // This should also help if there are many draw calls, and it also
    // should facilitate objects which use the same resources
    //
    // Best implementation of these would make the draw_X commands internal,
    // and instead provide an exposed "draw" function that instead
    // adds the draw command to a staging array.

    // TODO: Clay renderer should have an id: data unordered map to allow
    // for basic animations like css
    // Would also probably need to make a Tween class or just lerp it.

    ClayRenderer::render_commands(renderer, render_commands);
    SpriteSystem::draw_all(renderer);
    renderer.end_frame();
  }
  SDL_Log("Exiting...");
  cleanup();
  return 0;
}
