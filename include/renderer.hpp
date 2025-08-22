#pragma once

#define WIDTH 320
#define HEIGHT 180

#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL.h"
#include "SDL3_image/SDL_image.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

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

const int viewport_scale = 4;

struct SpriteFragmentUniformBuffer {
  glm::vec4 modulate;
  float time;
};

static SpriteFragmentUniformBuffer sprite_fragment_uniform_buffer{};

struct UIRectFragmentUniformBuffer {
  glm::vec4 modulate;
  float time;
};

static UIRectFragmentUniformBuffer ui_rect_fragment_uniform_buffer{};

struct VertexUniformBuffer {
  glm::mat4 mvp_matrix;
};

static VertexUniformBuffer vertex_uniform_buffer{};

// TODO: Interpolate proj matrix for smoother resizing
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
                   glm::vec2 scale);
  bool draw_rect(glm::vec2 position, glm::vec2 size, glm::vec4 color);
  bool cleanup();

private:
  Context context;

  // TODO: Have support for multiple pipelines
  // SDL_GPUGraphicsPipeline *graphics_pipeline;
  std::unordered_map<std::string, SDL_GPUGraphicsPipeline *> graphics_pipelines;
  std::unordered_map<std::string, SDL_GPUBuffer *> vertex_buffers;
  std::unordered_map<std::string, SDL_GPUBuffer *> index_buffers;
  std::unordered_map<std::string, SDL_GPUTexture *> gpu_textures;

  // TODO: Have support for multiple samplers
  SDL_GPUSampler *pixel_sampler;

  SDL_GPURenderPass *_render_pass;
  SDL_GPUCommandBuffer *_command_buffer;
};
