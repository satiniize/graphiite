#pragma once

#include <string>
#include <cstdint>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "glm/mat4x4.hpp"

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

const int WIDTH = 1280;
const int HEIGHT = 720;

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

class Renderer {
public:
  Uint32 width;
  Uint32 height;

  Renderer();
  ~Renderer();
  bool load_texture(std::string path, SDL_Surface *image_data);
  bool load_geometry(std::string path, const Vertex *vertices,
                     size_t vertex_size, const Uint16 *indices,
                     size_t index_size);
  bool create_graphics_pipeline(std::string path, SDL_GPUShader *vertex_shader,
                                SDL_GPUShader *fragment_shader);
  bool init();
  bool begin_frame();
  bool end_frame();
  // Drawing functions
  bool draw_sprite(std::string path, glm::vec2 translation, float rotation,
                   glm::vec2 scale, glm::vec4 color);
  bool draw_color_rect(glm::vec2 position, glm::vec2 size, glm::vec4 color,
                       glm::vec4 corner_radius);
  bool draw_texture_rect(std::string path, glm::vec2 position, glm::vec2 size,
                         glm::vec4 color, glm::vec4 corner_radius, bool tiling);
  bool draw_text(const char *text, int length, float point_size,
                 glm::vec2 position, glm::vec4 color);
  bool draw_arc(glm::vec2 position, float radius, float thickness,
                float rotation, glm::vec4 color);
  bool begin_scissor_mode(glm::ivec2 pos, glm::ivec2 size);
  bool end_scissor_mode();
  bool cleanup();
  glm::vec2 glyph_size;
  float font_sample_point_size = 64.0f;
  float viewport_scale = 2.0f;

private:
  Context context;

  std::unordered_map<std::string, SDL_GPUGraphicsPipeline *> graphics_pipelines;
  std::unordered_map<std::string, SDL_GPUBuffer *> vertex_buffers;
  std::unordered_map<std::string, SDL_GPUBuffer *> index_buffers;
  std::unordered_map<std::string, SDL_GPUTexture *> gpu_textures;

  // TODO: Have support for multiple samplers
  SDL_GPUSampler *clamp_sampler;
  SDL_GPUSampler *wrap_sampler;

  SDL_GPURenderPass *_render_pass;
  SDL_GPUCommandBuffer *_command_buffer;

  glm::mat4 projection_matrix;
};
