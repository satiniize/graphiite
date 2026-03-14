#pragma once

#include <cstdint>
#include <string>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <glm/mat4x4.hpp>

#include "texture.hpp"

struct Context {
  SDL_Window *window;
  SDL_GPUDevice *device;
  const char *title;
};

struct Vertex {
  float x, y, z;    // vec3 position
  float r, g, b, a; // vec4 color
  float u, v;       // vec2 texture coordinates
};

struct GlobalUniform {
  float time;
};

// Vertex uniform blocks
struct BasicVertexUniformBuffer {
  glm::mat4 mvp_matrix;
};

struct TextVertexUniformBuffer {
  glm::mat4 mvp_matrix;
  float time;
  float offset;
  float padding1;
  float padding2;
};

// Fragment uniform blocks
struct CommonFragmentUniformBlock {
  glm::vec4 modulate;
  float time;
};

struct SpriteFragmentUniformBuffer {
  glm::vec4 modulate;
  float time;
};

struct ColorRectFragmentUniformBuffer {
  glm::vec4 size;
  glm::vec4 modulate;
  glm::vec4 corner_radii;
};

struct TextureRectFragmentUniformBuffer {
  glm::vec4 size;
  glm::vec4 modulate;
  glm::vec4 corner_radii;
  uint32_t tiling;
};

struct TextFragmentUniformBuffer {
  glm::vec4 modulate;
  glm::vec4 uv_rect;
};

struct ArcFragmentUniformBuffer {
  glm::vec4 modulate;
  float radius;
  float thickness;
  float padding1;
  float padding2;
};

static BasicVertexUniformBuffer basic_vertex_uniform_buffer{};
static TextVertexUniformBuffer text_vertex_uniform_buffer{};

static SpriteFragmentUniformBuffer sprite_fragment_uniform_buffer{};
static ColorRectFragmentUniformBuffer color_rect_fragment_uniform_buffer{};
static TextureRectFragmentUniformBuffer texture_rect_fragment_uniform_buffer{};
static TextFragmentUniformBuffer text_fragment_uniform_buffer{};
static ArcFragmentUniformBuffer arc_fragment_uniform_buffer{};

using FontID = std::size_t;
using SamplerID = std::size_t;
using GeometryID = std::size_t;
using GraphicsPipelineID = std::size_t;

class Renderer {
public:
  uint32_t width;
  uint32_t height;

  Renderer(uint32_t width, uint32_t height);
  ~Renderer();
  // These are mature
  TextureID upload_texture(unsigned char *pixels, int w, int h);
  GeometryID upload_geometry(const Vertex *vertices, size_t vertex_size,
                             const Uint16 *indices, size_t index_size);
  GraphicsPipelineID create_graphics_pipeline(SDL_GPUShader *vertex_shader,
                                              SDL_GPUShader *fragment_shader);
  // TODO: These aren't mature, redesign please
  TextureID load_and_upload_ascii_font_atlas(
      const std::string &font_path); // TODO: Seperation of concerns
  void create_render_targets();      // TODO: Shotgun
  // Loop functions
  bool update_swapchain_texture(); // TODO: Not much of a descriptive name
  bool begin_frame();
  bool end_frame();
  // Drawing functions
  bool draw_sprite(TextureID texture_id, glm::vec2 translation, float rotation,
                   glm::vec2 scale, glm::vec4 color);
  bool draw_color_rect(glm::vec2 position, glm::vec2 size, glm::vec4 color,
                       glm::vec4 corner_radius);
  bool draw_texture_rect(TextureID texture_id, glm::vec2 position,
                         glm::vec2 size, glm::vec4 color,
                         glm::vec4 corner_radius, bool tiling);
  bool draw_text(const char *text, int length, float point_size,
                 glm::vec2 position, glm::vec4 color);
  bool draw_arc(glm::vec2 position, float radius, float thickness,
                float rotation, glm::vec4 color);
  // Scissor mode
  bool begin_scissor_mode(glm::ivec2 pos, glm::ivec2 size);
  bool end_scissor_mode();

  glm::vec2 glyph_size;
  float font_sample_point_size = 58.0f;
  float viewport_scale = 2.0f;

private:
  Context context;

  std::unordered_map<TextureID, SDL_GPUTexture *> gpu_textures;
  std::unordered_map<GeometryID, SDL_GPUBuffer *> vertex_buffers;
  std::unordered_map<GeometryID, SDL_GPUBuffer *> index_buffers;
  std::unordered_map<GraphicsPipelineID, SDL_GPUGraphicsPipeline *>
      graphics_pipelines;

  // TODO: Have support for multiple samplers
  SDL_GPUSampler *clamp_sampler;
  SDL_GPUSampler *wrap_sampler;

  SDL_GPURenderPass *_render_pass;
  SDL_GPUCommandBuffer *_command_buffer;

  SDL_GPUTexture *color_render_target;
  SDL_GPUTexture *resolve_target;
  SDL_GPUTexture *swapchain_texture;

  glm::mat4 projection_matrix;

  // GPU resource IDs
  TextureID next_texture_id = 0;
  GeometryID next_geometry_id = 0;
  GraphicsPipelineID next_pipeline_id = 0;

  // TextureID font_texture_id = -1;
  TextureID italic_font_atlas_id;
  TextureID regular_font_atlas_id;
  GeometryID quad_geometry_id;
  GraphicsPipelineID sprite_pipeline_id;
  GraphicsPipelineID color_rect_pipeline_id;
  GraphicsPipelineID texture_rect_pipeline_id;
  GraphicsPipelineID text_pipeline_id;
  GraphicsPipelineID arc_pipeline_id;

  SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;

  const std::string regular_font_path = "res/fonts/XanhMono-Regular.ttf";
  const std::string italic_font_path = "res/fonts/XanhMono-Italic.ttf";
};
