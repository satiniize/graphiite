#pragma once

#include <cstdint>
#include <string>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <glm/glm.hpp>

#include "font.hpp"
#include "image.hpp"
#include "texture.hpp"

struct Vertex {
  float x, y, z;    // vec3 Position
  float r, g, b, a; // vec4 Color
  float u, v;       // vec2 Texture Coordinates
};

using GeometryID = std::size_t;
using GraphicsPipelineID = std::size_t;
using ComputePipelineID = std::size_t;

struct GraphicsPipelineParams {
  int num_vertex_uniform_buffers = 0;
  int num_vertex_samplers = 0;
  int num_vertex_storage_buffers = 0;
  int num_vertex_storage_textures = 0;
  int num_fragment_uniform_buffers = 0;
  int num_fragment_samplers = 0;
  int num_fragment_storage_buffers = 0;
  int num_fragment_storage_textures = 0;
};

struct ComputePipelineParams {
  uint32_t num_samplers = 0;
  uint32_t num_readonly_storage_textures = 0;
  uint32_t num_readonly_storage_buffers = 0;
  uint32_t num_readwrite_storage_textures = 0;
  uint32_t num_readwrite_storage_buffers = 0;
  uint32_t num_uniform_buffers = 0;
};

// TODO: Maybe switch this to clay vectors?
struct RectParams {
  // Fundamentals
  glm::vec2 position;
  glm::vec2 size;
  glm::vec4 color;
  glm::vec4 corner_radii = glm::vec4(0.0f);
  float corner_exponent = 2.0f;
  // Texture
  bool use_texture = false;
  TextureID texture_id = -1;
  bool tiling = false;
  // Stroke
  bool draw_stroke = false;
  glm::vec4 stroke_thickness = glm::vec4(0.0f);
  // Smoothing | Used for shadow rendering
  float smoothing = 1.0f;
};

struct TextParams {
  std::string text;
  int point_size = 1;
  glm::vec2 position;
  glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

enum class BoundPipeline {
  NONE,
  RECT,
  TEXT,
};

class Renderer {
public:
  uint32_t width;
  uint32_t height;

  Renderer(std::string name, uint32_t width, uint32_t height);
  ~Renderer();

  void create_render_targets(); // Shorthand for first init and window resize

  // Textures
  Texture upload_texture(const Image &image);
  Image download_texture(const Texture &texture);

  Texture create_compute_target_texture(int w, int h, bool is_sampler = false);

  // Geometry
  GeometryID upload_geometry(const Vertex *vertices, size_t vertex_size,
                             const Uint16 *indices, size_t index_size);

  // Pipelines
  GraphicsPipelineID create_graphics_pipeline(GraphicsPipelineParams params,
                                              std::vector<char> vertex_code,
                                              std::vector<char> fragment_code);
  ComputePipelineID create_compute_pipeline(ComputePipelineParams params,
                                            std::vector<char> compute_code);

  // Passes
  void start_frame();
  void end_frame();
  // Compute
  bool begin_compute_pass(Texture film_target_texture);
  bool end_compute_pass();
  // Render
  bool begin_render_pass();
  bool end_render_pass();

  // Compute functions
  bool compute_film(void *fragment_uniforms_ptr, size_t fragment_uniforms_size,
                    Texture film_source_texture, Texture film_target_texture);

  // Drawing functions
  void draw_rect(RectParams params);
  void draw_text(TextParams params);
  // Scissor mode
  void begin_scissor(glm::ivec2 pos, glm::ivec2 size);
  void end_scissor();

  float viewport_scale = 1.0f;

  Font default_font;

private:
  std::string _title;
  SDL_Window *_window;
  SDL_GPUDevice *_device;

  // TODO: Input file path sanitizing
  const std::string regular_font_path =
      "assets/fonts/AtkinsonHyperlegibleNext-Medium.ttf";

  // TODO: Pipelines need to be recreated when the sample count changes
  SDL_GPUSampleCount _sample_count = SDL_GPU_SAMPLECOUNT_1;

  BoundPipeline _bound_pipeline = BoundPipeline::NONE;

  TextureID _next_texture_id = 0;
  GeometryID _next_geometry_id = 0;
  ComputePipelineID _next_compute_pipeline_id = 0;
  GraphicsPipelineID _next_graphics_pipeline_id = 0;

  std::unordered_map<TextureID, SDL_GPUTexture *> _gpu_textures;
  std::unordered_map<GeometryID, SDL_GPUBuffer *> _vertex_buffers;
  std::unordered_map<GeometryID, SDL_GPUBuffer *> _index_buffers;
  std::unordered_map<ComputePipelineID, SDL_GPUComputePipeline *>
      _compute_pipelines;
  std::unordered_map<GraphicsPipelineID, SDL_GPUGraphicsPipeline *>
      _graphics_pipelines;

  // TODO: Have support for multiple samplers
  SDL_GPUSampler *_clamp_sampler;
  SDL_GPUSampler *_wrap_sampler;

  // TODO: Really iffy way to handle the main render pass
  // Should do a queue instead and batch together
  SDL_GPURenderPass *_render_pass;
  SDL_GPUComputePass *_compute_pass;
  SDL_GPUCommandBuffer *_command_buffer;

  SDL_GPUTexture *_color_render_target;
  SDL_GPUTexture *_resolve_target;
  SDL_GPUTexture *_swapchain_texture;

  glm::mat4 _projection_matrix;

  Texture _dummy_texture;

  GeometryID _quad_geometry_id;

  GraphicsPipelineID _sdf_rect_stroke_pipeline_id;
  GraphicsPipelineID _sprite_pipeline_id;
  GraphicsPipelineID _text_pipeline_id;
  GraphicsPipelineID _film_pipeline_id;

  const uint32_t _threadcount_x = 16;
  const uint32_t _threadcount_y = 16;
  const uint32_t _threadcount_z = 1;
};
