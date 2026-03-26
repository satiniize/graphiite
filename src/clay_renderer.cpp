#include "clay_renderer.hpp"
#include "renderer.hpp"

#include <sys/types.h>

#include <SDL3/SDL_log.h>

// TODO: Unsure if this and clay manager should be combined or not, probably not
namespace ClayRenderer {
void render_commands(Renderer &renderer,
                     Clay_RenderCommandArray render_commands) {
  for (int i = 0; i < render_commands.length; i++) {
    Clay_RenderCommand *render_command =
        Clay_RenderCommandArray_Get(&render_commands, i);

    const Clay_BoundingBox bounding_box = render_command->boundingBox;
    const SDL_FRect rect = {bounding_box.x, bounding_box.y, bounding_box.width,
                            bounding_box.height};
    const uint16_t z_index = static_cast<uint16_t>(render_command->zIndex);
    // TODO: Properly clamp corner radii by ratio
    switch (render_command->commandType) {
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
      Clay_RectangleRenderData *render_data_rectangle =
          &render_command->renderData.rectangle;
      glm::vec4 color((float)render_data_rectangle->backgroundColor.r / 255.0f,
                      (float)render_data_rectangle->backgroundColor.g / 255.0f,
                      (float)render_data_rectangle->backgroundColor.b / 255.0f,
                      (float)render_data_rectangle->backgroundColor.a / 255.0f);
      glm::vec4 corner_radii(render_data_rectangle->cornerRadius.topLeft,
                             render_data_rectangle->cornerRadius.topRight,
                             render_data_rectangle->cornerRadius.bottomLeft,
                             render_data_rectangle->cornerRadius.bottomRight);
      RectParams rect_params = {
          .position = glm::vec2(rect.x, rect.y),
          .size = glm::vec2(rect.w, rect.h),
          .color = color,
          .corner_radii = corner_radii,
          .use_texture = false,
          .texture_id = 0,
          .tiling = false,
          .draw_stroke = false,
          .stroke_thickness = glm::vec4(0.0f),
      };
      renderer.draw_rect(rect_params);
    } break;
    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
      Clay_TextRenderData *render_data_text = &render_command->renderData.text;

      // Get variables
      const char *chars = render_data_text->stringContents.chars;
      uint32_t length =
          static_cast<uint32_t>(render_data_text->stringContents.length);
      uint16_t font_size = render_data_text->fontSize;

      glm::vec4 color((float)render_data_text->textColor.r / 255.0f,
                      (float)render_data_text->textColor.g / 255.0f,
                      (float)render_data_text->textColor.b / 255.0f,
                      (float)render_data_text->textColor.a / 255.0f);

      // Draw text
      renderer.draw_text(chars, length, font_size, glm::vec2(rect.x, rect.y),
                         color);
    } break;
    case CLAY_RENDER_COMMAND_TYPE_BORDER: {
      Clay_BorderRenderData *render_data_border =
          &render_command->renderData.border;
      const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;

      glm::vec4 corner_radii = {
          render_data_border->cornerRadius.topLeft,
          render_data_border->cornerRadius.topRight,
          render_data_border->cornerRadius.bottomLeft,
          render_data_border->cornerRadius.bottomRight,
      };
      glm::vec4 color((float)render_data_border->color.r / 255.0f,
                      (float)render_data_border->color.g / 255.0f,
                      (float)render_data_border->color.b / 255.0f,
                      (float)render_data_border->color.a / 255.0f);
      // left, right, top, bottom
      glm::vec4 stroke_thickness((float)render_data_border->width.left,
                                 (float)render_data_border->width.right,
                                 (float)render_data_border->width.top,
                                 (float)render_data_border->width.bottom);
      RectParams rect_params = {
          .position = glm::vec2(rect.x, rect.y),
          .size = glm::vec2(rect.w, rect.h),
          .color = color,
          .corner_radii = corner_radii,
          .use_texture = false,
          .texture_id = 0,
          .tiling = false,
          .draw_stroke = true,
          .stroke_thickness = stroke_thickness,
      };
      renderer.draw_rect(rect_params);
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
      Clay_ImageRenderData *render_data_image =
          &render_command->renderData.image;

      // Get variables
      glm::vec4 modulate_color = {
          render_data_image->backgroundColor.r / 255.0f,
          render_data_image->backgroundColor.g / 255.0f,
          render_data_image->backgroundColor.b / 255.0f,
          render_data_image->backgroundColor.a / 255.0f,
      };
      Texture *data_pointer =
          static_cast<Texture *>(render_data_image->imageData); // Get pointer
      Texture image_data = *data_pointer; // Dereference pointer
      glm::vec4 corner_radii = {
          render_data_image->cornerRadius.topLeft,
          render_data_image->cornerRadius.topRight,
          render_data_image->cornerRadius.bottomLeft,
          render_data_image->cornerRadius.bottomRight,
      };

      RectParams rect_params = {
          .position = glm::vec2(rect.x, rect.y),
          .size = glm::vec2(rect.w, rect.h),
          .color = modulate_color,
          .corner_radii = corner_radii,
          .use_texture = true,
          .texture_id = image_data.id,
          .tiling = image_data.tiling,
          .draw_stroke = false,
          .stroke_thickness = glm::vec4(0.0f),
      };
      renderer.draw_rect(rect_params);

    } break;
    default:
      SDL_Log("Unknown render command type: %d", render_command->commandType);
    }
  }
}
} // namespace ClayRenderer
