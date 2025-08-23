#include "clay_renderer.hpp"

namespace ClayRenderer {
static void render_filled_rounded_rect(const SDL_FRect rect,
                                       const float cornerRadius,
                                       const Clay_Color _color) {
  const SDL_FColor color = {_color.r / 255, _color.g / 255, _color.b / 255,
                            _color.a / 255};

  int index_count = 0;
  int vertex_count = 0;

  const float min_radius = SDL_min(rect.w, rect.h) / 2.0f;
  const float clamped_radius = SDL_min(cornerRadius, min_radius);

  const int num_circle_segments =
      SDL_max(NUM_CIRCLE_SEGMENTS, (int)clamped_radius * 0.5f);

  int total_vertices = 4 + (4 * (num_circle_segments * 2)) + 2 * 4;
  int total_indices = 6 + (4 * (num_circle_segments * 3)) + 6 * 4;

  Vertex vertices[total_vertices];
  Uint16 indices[total_indices];

  // define center rectangle
  vertices[vertex_count++] = (Vertex){
      // position
      rect.x + clamped_radius,
      rect.y + clamped_radius,
      0.0f,
      // color
      color.r,
      color.g,
      color.b,
      color.a,
      // uv
      0.0f,
      0.0f,
  }; // 0 center TL
  vertices[vertex_count++] = (Vertex){
      rect.x + rect.w - clamped_radius,
      rect.y + clamped_radius,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      1.0f,
      0.0f,
  }; // 1 center TR
  vertices[vertex_count++] = (Vertex){
      rect.x + rect.w - clamped_radius,
      rect.y + rect.h - clamped_radius,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      1.0f,
      1.0f,
  }; // 2 center BR
  vertices[vertex_count++] = (Vertex){
      rect.x + clamped_radius,
      rect.y + rect.h - clamped_radius,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      0.0f,
      1.0f,
  }; // 3 center BL

  indices[index_count++] = 0;
  indices[index_count++] = 1;
  indices[index_count++] = 3;
  indices[index_count++] = 1;
  indices[index_count++] = 2;
  indices[index_count++] = 3;

  // define rounded corners as triangle fans
  const float step = (SDL_PI_F / 2) / num_circle_segments;
  for (int i = 0; i < num_circle_segments; i++) {
    const float angle1 = (float)i * step;
    const float angle2 = ((float)i + 1.0f) * step;

    for (int j = 0; j < 4; j++) { // Iterate over four corners
      float cx, cy, signX, signY;

      switch (j) {
      case 0:
        cx = rect.x + clamped_radius;
        cy = rect.y + clamped_radius;
        signX = -1;
        signY = -1;
        break; // Top-left
      case 1:
        cx = rect.x + rect.w - clamped_radius;
        cy = rect.y + clamped_radius;
        signX = 1;
        signY = -1;
        break; // Top-right
      case 2:
        cx = rect.x + rect.w - clamped_radius;
        cy = rect.y + rect.h - clamped_radius;
        signX = 1;
        signY = 1;
        break; // Bottom-right
      case 3:
        cx = rect.x + clamped_radius;
        cy = rect.y + rect.h - clamped_radius;
        signX = -1;
        signY = 1;
        break; // Bottom-left
      default:
        return;
      }

      vertices[vertex_count++] = (Vertex){
          cx + SDL_cosf(angle1) * clamped_radius * signX,
          cy + SDL_sinf(angle1) * clamped_radius * signY,
          0.0f,
          color.r,
          color.g,
          color.b,
          color.a,
          0.0f,
          0.0f,
      };
      vertices[vertex_count++] = (Vertex){
          cx + SDL_cosf(angle2) * clamped_radius * signX,
          cy + SDL_sinf(angle2) * clamped_radius * signY,
          0.0f,
          color.r,
          color.g,
          color.b,
          color.a,
          0.0f,
          0.0f,
      };

      indices[index_count++] =
          j; // Connect to corresponding central rectangle vertex
      indices[index_count++] = vertex_count - 2;
      indices[index_count++] = vertex_count - 1;
    }
  }

  // Define edge rectangles
  //  Top edge
  vertices[vertex_count++] = (Vertex){
      rect.x + clamped_radius,
      rect.y,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      0.0f,
      0.0f,
  }; // TL
  vertices[vertex_count++] = (Vertex){
      rect.x + rect.w - clamped_radius,
      rect.y,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      1.0f,
      0.0f,
  }; // TR

  indices[index_count++] = 0;
  indices[index_count++] = vertex_count - 2; // TL
  indices[index_count++] = vertex_count - 1; // TR
  indices[index_count++] = 1;
  indices[index_count++] = 0;
  indices[index_count++] = vertex_count - 1; // TR
  // Right edge
  vertices[vertex_count++] = (Vertex){
      rect.x + rect.w,
      rect.y + clamped_radius,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      1.0f,
      0.0f,
  }; // RT
  vertices[vertex_count++] = (Vertex){
      rect.x + rect.w,
      rect.y + rect.h - clamped_radius,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      1.0f,
      1.0f,
  }; // RB

  indices[index_count++] = 1;
  indices[index_count++] = vertex_count - 2; // RT
  indices[index_count++] = vertex_count - 1; // RB
  indices[index_count++] = 2;
  indices[index_count++] = 1;
  indices[index_count++] = vertex_count - 1; // RB
  // Bottom edge
  vertices[vertex_count++] = (Vertex){
      rect.x + rect.w - clamped_radius,
      rect.y + rect.h,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      1.0f,
      1.0f,
  }; // BR
  vertices[vertex_count++] = (Vertex){
      rect.x + clamped_radius,
      rect.y + rect.h,
      0.0f,
      color.r,
      color.g,
      color.b,
      color.a,
      0.0f,
      1.0f,
  }; // BL

  indices[index_count++] = 2;
  indices[index_count++] = vertex_count - 2; // BR
  indices[index_count++] = vertex_count - 1; // BL
  indices[index_count++] = 3;
  indices[index_count++] = 2;
  indices[index_count++] = vertex_count - 1; // BL
  // Left edge
  vertices[vertex_count++] = (Vertex){
      rect.x,
      rect.y + rect.h - clamped_radius,
      0.0f,
      // color
      color.r,
      color.g,
      color.b,
      color.a,
      0.0f,
      1.0f,
  }; // LB
  vertices[vertex_count++] = (Vertex){
      rect.x,
      rect.y + clamped_radius,
      0.0f,
      // color
      color.r,
      color.g,
      color.b,
      color.a,
      0.0f,
      0.0f,
  }; // LT

  indices[index_count++] = 3;
  indices[index_count++] = vertex_count - 2; // LB
  indices[index_count++] = vertex_count - 1; // LT
  indices[index_count++] = 0;
  indices[index_count++] = 3;
  indices[index_count++] = vertex_count - 1; // LT

  // Render everything
  // SDL_RenderGeometry(rendererData->renderer, NULL, vertices, vertex_count,
  //                    indices, index_count);
}

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
      if (config->cornerRadius.topLeft > 0) {
      } else {
        renderer.draw_rect(glm::vec2(rect.x, rect.y), glm::vec2(rect.w, rect.h),
                           color);
      }
    } break;
    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
      Clay_TextRenderData *config = &renderCommand->renderData.text;
      renderer.draw_text(config->stringContents.chars,
                         glm::vec2(rect.x, rect.y));
      // TTF_Font *font = rendererData->fonts[config->fontId];
      // TTF_SetFontSize(font, config->fontSize);
      // TTF_Text *text = TTF_CreateText(rendererData->textEngine, font,
      // config->stringContents.chars,
      // config->stringContents.length);
      // TTF_SetTextColor(text, config->textColor.r, config->textColor.g,
      // config->textColor.b, config->textColor.a);
      // TTF_DrawRendererText(text, rect.x, rect.y);
      // TTF_DestroyText(text);
    } break;

    default:
      SDL_Log("Unknown render command type: %d", renderCommand->commandType);
    }
  }
}
} // namespace ClayRenderer
