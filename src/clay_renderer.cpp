#include "clay_renderer.hpp"
#include <sys/types.h>

namespace ClayRenderer {
void render_commands(Renderer &renderer,
                     Clay_RenderCommandArray render_commands) {
  for (int i = 0; i < render_commands.length; i++) {
    Clay_RenderCommand *render_command =
        Clay_RenderCommandArray_Get(&render_commands, i);

    const Clay_BoundingBox bounding_box = render_command->boundingBox;
    const SDL_FRect rect = {bounding_box.x, bounding_box.y, bounding_box.width,
                            bounding_box.height};

    switch (render_command->commandType) {
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
      Clay_RectangleRenderData *render_data_rectangle =
          &render_command->renderData.rectangle;

      // Get variables
      glm::vec4 color((float)render_data_rectangle->backgroundColor.r / 255.0f,
                      (float)render_data_rectangle->backgroundColor.g / 255.0f,
                      (float)render_data_rectangle->backgroundColor.b / 255.0f,
                      (float)render_data_rectangle->backgroundColor.a / 255.0f);
      glm::vec4 corner_radii(render_data_rectangle->cornerRadius.topLeft,
                             render_data_rectangle->cornerRadius.topRight,
                             render_data_rectangle->cornerRadius.bottomLeft,
                             render_data_rectangle->cornerRadius.bottomRight);

      // Draw the rect
      renderer.draw_color_rect(glm::vec2(rect.x, rect.y),
                               glm::vec2(rect.w, rect.h), color, corner_radii);
    } break;
    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
      // TODO: Implement and improve text rendering
      // // A string slice containing the text to be rendered.
      // // Note: this is not guaranteed to be null terminated.
      // Clay_StringSlice stringContents;
      // // Conventionally represented as 0-255 for each channel, but
      // interpretation is up to the renderer. Clay_Color textColor;
      // // An integer representing the font to use to render this text,
      // transparently passed through from the text declaration. uint16_t
      // fontId; uint16_t fontSize;
      // // Specifies the extra whitespace gap in pixels between each character.
      // uint16_t letterSpacing;
      // // The height of the bounding box for this line of text.
      // uint16_t lineHeight;

      Clay_TextRenderData *render_data_text = &render_command->renderData.text;

      // Get variables
      const char *chars = render_data_text->stringContents.chars;
      uint32_t length =
          static_cast<uint32_t>(render_data_text->stringContents.length);
      uint16_t font_size = render_data_text->fontSize;

      // Draw text
      renderer.draw_text(chars, length, font_size, glm::vec2(rect.x, rect.y));
    } break;
    case CLAY_RENDER_COMMAND_TYPE_BORDER: {
      Clay_BorderRenderData *render_data_border =
          &render_command->renderData.border;
      const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
      const Clay_CornerRadius clampedRadii = {
          .topLeft =
              SDL_min(render_data_border->cornerRadius.topLeft, minRadius),
          .topRight =
              SDL_min(render_data_border->cornerRadius.topRight, minRadius),
          .bottomLeft =
              SDL_min(render_data_border->cornerRadius.bottomLeft, minRadius),
          .bottomRight =
              SDL_min(render_data_border->cornerRadius.bottomRight, minRadius)};
      glm::vec4 color((float)render_data_border->color.r / 255.0f,
                      (float)render_data_border->color.g / 255.0f,
                      (float)render_data_border->color.b / 255.0f,
                      (float)render_data_border->color.a / 255.0f);

      // Left edge
      if (render_data_border->width.left > 0) {
        const float starting_y = rect.y + clampedRadii.topLeft;
        const float length =
            rect.h - clampedRadii.topLeft - clampedRadii.bottomLeft;

        renderer.draw_color_rect(
            glm::vec2(rect.x, starting_y),
            glm::vec2(render_data_border->width.left, length), color,
            glm::vec4(0.0f));
      }
      // Right edge
      if (render_data_border->width.right > 0) {
        const float starting_x =
            rect.x + rect.w - (float)render_data_border->width.right;
        const float starting_y = rect.y + clampedRadii.topRight;
        const float length =
            rect.h - clampedRadii.topRight - clampedRadii.bottomRight;
        renderer.draw_color_rect(
            glm::vec2(starting_x, starting_y),
            glm::vec2(render_data_border->width.right, length), color,
            glm::vec4(0.0f));
      }
      // Top edge
      if (render_data_border->width.top > 0) {
        const float starting_x = rect.x + clampedRadii.topLeft;
        const float length =
            rect.w - clampedRadii.topLeft - clampedRadii.topRight;

        renderer.draw_color_rect(
            glm::vec2(starting_x, rect.y),
            glm::vec2(length, render_data_border->width.top), color,
            glm::vec4(0.0f));
      }
      // Bottom edge
      if (render_data_border->width.bottom > 0) {
        const float starting_x = rect.x + clampedRadii.bottomLeft;
        const float starting_y =
            rect.y + rect.h - (float)render_data_border->width.bottom;
        const float length =
            rect.w - clampedRadii.bottomLeft - clampedRadii.bottomRight;

        renderer.draw_color_rect(
            glm::vec2(starting_x, starting_y),
            glm::vec2(length, render_data_border->width.bottom), color,
            glm::vec4(0.0f));
      }

      if (render_data_border->cornerRadius.topLeft > 0) {
        const float centerX = rect.x + clampedRadii.topLeft;
        const float centerY = rect.y + clampedRadii.topLeft;
        renderer.draw_arc(glm::vec2(centerX, centerY), clampedRadii.topLeft,
                          render_data_border->width.top, 90.0f, color);
      }
      if (render_data_border->cornerRadius.topRight > 0) {
        const float centerX = rect.x + rect.w - clampedRadii.topRight;
        const float centerY = rect.y + clampedRadii.topRight;
        renderer.draw_arc(glm::vec2(centerX, centerY), clampedRadii.topRight,
                          render_data_border->width.top, 0.0f, color);
      }
      if (render_data_border->cornerRadius.bottomLeft > 0) {
        const float centerX = rect.x + clampedRadii.bottomLeft;
        const float centerY = rect.y + rect.h - clampedRadii.bottomLeft;
        renderer.draw_arc(glm::vec2(centerX, centerY), clampedRadii.bottomLeft,
                          render_data_border->width.bottom, 180.0f, color);
      }
      if (render_data_border->cornerRadius.bottomRight > 0) {
        const float centerX = rect.x + rect.w - clampedRadii.bottomRight;
        const float centerY = rect.y + rect.h - clampedRadii.bottomRight;
        renderer.draw_arc(glm::vec2(centerX, centerY), clampedRadii.bottomRight,
                          render_data_border->width.bottom, 270.0f, color);
      }
    } break;
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
      // TODO: Investigate this weird off by one pixel error
      renderer.begin_scissor_mode(
          glm::ivec2(bounding_box.x * renderer.viewport_scale,
                     bounding_box.y * renderer.viewport_scale),
          glm::ivec2(bounding_box.width * renderer.viewport_scale + 1,
                     bounding_box.height * renderer.viewport_scale + 1));
    } break;
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
      renderer.end_scissor_mode();
    } break;
    case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
      // TODO: Support rounded corners
      // TODO: In image data should have settings for tiling or stretched
      Clay_ImageRenderData *render_data_image =
          &render_command->renderData.image;

      // Get variables
      glm::vec4 modulate_color = {
          render_data_image->backgroundColor.r / 255.0f,
          render_data_image->backgroundColor.g / 255.0f,
          render_data_image->backgroundColor.b / 255.0f,
          render_data_image->backgroundColor.a / 255.0f,
      };
      ImageData *data_pointer =
          static_cast<ImageData *>(render_data_image->imageData); // Get pointer
      ImageData image_data = *data_pointer; // Dereference pointer
      glm::vec4 corner_radii = {
          render_data_image->cornerRadius.topLeft,
          render_data_image->cornerRadius.topRight,
          render_data_image->cornerRadius.bottomLeft,
          render_data_image->cornerRadius.bottomRight,
      };

      // renderer.draw_sprite(
      //     image_path, glm::vec2(rect.x + rect.w / 2.0f, rect.y + rect.h
      //     / 2.0f), 0.0f, glm::vec2(rect.w, rect.h), glm::vec4(1.0f));

      // Draw the image
      renderer.draw_texture_rect(image_data.path, glm::vec2(rect.x, rect.y),
                                 glm::vec2(rect.w, rect.h), modulate_color,
                                 corner_radii, image_data.tiling);
    } break;
    default:
      SDL_Log("Unknown render command type: %d", render_command->commandType);
    }
  }
}
} // namespace ClayRenderer
