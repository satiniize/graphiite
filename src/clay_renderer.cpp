#include "clay_renderer.hpp"

namespace ClayRenderer {
void render_commands(Renderer &renderer,
                     Clay_RenderCommandArray renderCommands) {
  for (int i = 0; i < renderCommands.length; i++) {
    Clay_RenderCommand *renderCommand =
        Clay_RenderCommandArray_Get(&renderCommands, i);
    const Clay_BoundingBox bounding_box = renderCommand->boundingBox;
    const SDL_FRect rect = {bounding_box.x, bounding_box.y, bounding_box.width,
                            bounding_box.height};

    switch (renderCommand->commandType) {
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
      Clay_RectangleRenderData *config = &renderCommand->renderData.rectangle;
      glm::vec4 color((float)config->backgroundColor.r / 255.0f,
                      (float)config->backgroundColor.g / 255.0f,
                      (float)config->backgroundColor.b / 255.0f,
                      (float)config->backgroundColor.a / 255.0f);
      glm::vec4 corner_radii(
          config->cornerRadius.topLeft, config->cornerRadius.topRight,
          config->cornerRadius.bottomLeft, config->cornerRadius.bottomRight);
      renderer.draw_rect(glm::vec2(rect.x, rect.y), glm::vec2(rect.w, rect.h),
                         color, corner_radii);
    } break;
    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
      Clay_TextRenderData *config = &renderCommand->renderData.text;
      renderer.draw_text(config->stringContents.chars, config->fontSize,
                         glm::vec2(rect.x, rect.y));
    } break;
    case CLAY_RENDER_COMMAND_TYPE_BORDER: {
      Clay_BorderRenderData *config = &renderCommand->renderData.border;
      const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
      const Clay_CornerRadius clampedRadii = {
          .topLeft = SDL_min(config->cornerRadius.topLeft, minRadius),
          .topRight = SDL_min(config->cornerRadius.topRight, minRadius),
          .bottomLeft = SDL_min(config->cornerRadius.bottomLeft, minRadius),
          .bottomRight = SDL_min(config->cornerRadius.bottomRight, minRadius)};
      glm::vec4 color(
          (float)config->color.r / 255.0f, (float)config->color.g / 255.0f,
          (float)config->color.b / 255.0f, (float)config->color.a / 255.0f);

      // Left edge
      if (config->width.left > 0) {
        const float starting_y = rect.y + clampedRadii.topLeft;
        const float length =
            rect.h - clampedRadii.topLeft - clampedRadii.bottomLeft;

        renderer.draw_rect(glm::vec2(rect.x, starting_y),
                           glm::vec2(config->width.left, length), color,
                           glm::vec4(0.0f));
      }
      // Right edge
      if (config->width.right > 0) {
        const float starting_x = rect.x + rect.w - (float)config->width.right;
        const float starting_y = rect.y + clampedRadii.topRight;
        const float length =
            rect.h - clampedRadii.topRight - clampedRadii.bottomRight;
        renderer.draw_rect(glm::vec2(starting_x, starting_y),
                           glm::vec2(config->width.right, length), color,
                           glm::vec4(0.0f));
      }
      // Top edge
      if (config->width.top > 0) {
        const float starting_x = rect.x + clampedRadii.topLeft;
        const float length =
            rect.w - clampedRadii.topLeft - clampedRadii.topRight;

        renderer.draw_rect(glm::vec2(starting_x, rect.y),
                           glm::vec2(length, config->width.top), color,
                           glm::vec4(0.0f));
      }
      // Bottom edge
      if (config->width.bottom > 0) {
        const float starting_x = rect.x + clampedRadii.bottomLeft;
        const float starting_y = rect.y + rect.h - (float)config->width.bottom;
        const float length =
            rect.w - clampedRadii.bottomLeft - clampedRadii.bottomRight;

        renderer.draw_rect(glm::vec2(starting_x, starting_y),
                           glm::vec2(length, config->width.bottom), color,
                           glm::vec4(0.0f));
      }

      if (config->cornerRadius.topLeft > 0) {
        const float centerX = rect.x + clampedRadii.topLeft;
        const float centerY = rect.y + clampedRadii.topLeft;
        renderer.draw_rounded_corner_border(glm::vec2(centerX, centerY),
                                            clampedRadii.topLeft,
                                            config->width.top, 90.0f, color);
      }
      if (config->cornerRadius.topRight > 0) {
        const float centerX = rect.x + rect.w - clampedRadii.topRight;
        const float centerY = rect.y + clampedRadii.topRight;
        renderer.draw_rounded_corner_border(glm::vec2(centerX, centerY),
                                            clampedRadii.topRight,
                                            config->width.top, 0.0f, color);
      }
      if (config->cornerRadius.bottomLeft > 0) {
        const float centerX = rect.x + clampedRadii.bottomLeft;
        const float centerY = rect.y + rect.h - clampedRadii.bottomLeft;
        renderer.draw_rounded_corner_border(
            glm::vec2(centerX, centerY), clampedRadii.bottomLeft,
            config->width.bottom, 180.0f, color);
      }
      if (config->cornerRadius.bottomRight > 0) {
        const float centerX = rect.x + rect.w - clampedRadii.bottomRight;
        const float centerY = rect.y + rect.h - clampedRadii.bottomRight;
        renderer.draw_rounded_corner_border(
            glm::vec2(centerX, centerY), clampedRadii.bottomRight,
            config->width.bottom, 270.0f, color);
      }

    } break;
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
      renderer.begin_scissor_mode(
          glm::ivec2(bounding_box.x, bounding_box.y),
          glm::ivec2(bounding_box.width, bounding_box.height));
    } break;
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
      renderer.end_scissor_mode();
    } break;
    case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
      std::string *string_pointer =
          static_cast<std::string *>(renderCommand->renderData.image.imageData);
      std::string image_path = *string_pointer;
      renderer.draw_sprite(
          image_path, glm::vec2(rect.x + rect.w / 2.0f, rect.y + rect.h / 2.0f),
          0.0f, glm::vec2(rect.w, rect.h));
    } break;
    default:
      SDL_Log("Unknown render command type: %d", renderCommand->commandType);
    }
  }
}
} // namespace ClayRenderer
