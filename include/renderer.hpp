#pragma once

#include <cstdint>
#include <memory>
#include <queue>
#include <string>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <glm/mat4x4.hpp>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

#include "image.hpp"
#include "texture.hpp"

enum class TextureUsage {
  SAMPLER = SDL_GPU_TEXTUREUSAGE_SAMPLER, // For sampling
  COLOR_TARGET = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
  DEPTH_STENCIL_TARGET = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
  GRAPHICS_STORAGE_READ = SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ,
  COMPUTE_STORAGE_READ = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ,
  COMPUTE_STORAGE_WRITE = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE,
  COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE =
      SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE,
};

// struct Context {
//   SDL_Window *window;
//   SDL_GPUDevice *device;
//   const char *title;
// };

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
struct CommonFragmentUniformBuffer {
  float time;
};

struct SpriteFragmentUniformBuffer {
  glm::vec4 modulate;
};

struct TextFragmentUniformBuffer {
  glm::vec4 modulate;
  glm::vec4 uv_rect;
};

struct SDFRectStrokeFragmentUniformBuffer {
  glm::vec4 size;
  glm::vec4 modulate;
  glm::vec4 corner_radii;
  glm::vec4 inner_corner_radii;
  glm::vec4 stroke_thickness;
  uint32_t tiling;
  uint32_t use_texture;
  uint32_t draw_stroke;
  float smoothing = 1.0f;
};

struct RawProcessorFragmentUniformBuffer {
  glm::mat4 correction_matrix;
  glm::mat4 crosstalk_matrix;
  glm::mat4 dye_matrix;
  glm::vec4 d_min;
  glm::vec4 d_max;
  glm::vec4 k;
  glm::vec4 x0;
  float exposure_compensation;
};

static BasicVertexUniformBuffer basic_vertex_uniform_buffer{};
static TextVertexUniformBuffer text_vertex_uniform_buffer{};

static CommonFragmentUniformBuffer common_fragment_uniform_buffer{};
static SpriteFragmentUniformBuffer sprite_fragment_uniform_buffer{};
static TextFragmentUniformBuffer text_fragment_uniform_buffer{};

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

struct RectParams {
  // Fundamentals
  glm::vec2 position;
  glm::vec2 size;
  glm::vec4 color;
  glm::vec4 corner_radii = glm::vec4(0.0f);
  // Texture
  bool use_texture = false;
  TextureID texture_id = -1;
  bool tiling = false;
  // Stroke
  bool draw_stroke = false;
  glm::vec4 stroke_thickness = glm::vec4(0.0f);
  // Smoothing
  float smoothing = 1.0f;
};

struct SpriteParams {
  TextureID texture_id;
  glm::vec2 translation;
  float rotation;
  glm::vec2 scale;
  glm::vec4 color;
};

struct TextParams {
  std::string text;
  float point_size;
  glm::vec2 position;
  glm::vec4 color;
};

struct DrawCall {
  SpriteParams sprite_data;
  RectParams rect_data;
  TextParams text_data;
};

struct GlyphMetrics {
  glm::vec4 uv_rect; // normalized 0..1 atlas UVs (x0, y0, x1, y1)
  glm::vec2 size;    // bitmap size in pixels
  glm::vec2 bearing; // offset from cursor origin to top-left of bitmap
  float advance;     // cursor advance in pixels
};

// TODO: When renderer changes, sprite renderer and clay renderer needs to
// recompile
class Renderer {
public:
  uint32_t width;
  uint32_t height;

  Renderer(std::string name, uint32_t width, uint32_t height);
  ~Renderer();

  TextureID create_render_target(int w, int h);

  Texture upload_texture(const Image &image);
  Image download_texture(Texture &texture);
  void blit_texture(TextureID src, TextureID dst);

  GeometryID upload_geometry(const Vertex *vertices, size_t vertex_size,
                             const Uint16 *indices, size_t index_size);

  GraphicsPipelineID create_graphics_pipeline(GraphicsPipelineParams params,
                                              std::vector<char> vertex_code,
                                              std::vector<char> fragment_code);

  ComputePipelineID create_compute_pipeline(ComputePipelineParams params,
                                            std::vector<char> compute_code);

  TextureID load_and_upload_ascii_font_atlas(const std::string &font_path);

  void create_render_targets(); // Shorthand for first init and window resize
  // Passes
  void start_frame();
  void end_frame();

  bool begin_compute_pass(Texture film_target_texture);
  bool end_compute_pass();

  bool begin_render_pass();
  bool end_render_pass();
  // Compute functions
  bool compute_film(RawProcessorFragmentUniformBuffer fragment_uniforms,
                    Texture film_source_texture, Texture film_target_texture);
  // Drawing functions
  bool draw_sprite(TextureID texture_id, glm::vec2 translation, float rotation,
                   glm::vec2 scale, glm::vec4 color);
  bool draw_rect(RectParams params);
  bool draw_text(const char *text, int length, float point_size,
                 glm::vec2 position, glm::vec4 color);
  // Scissor mode
  bool begin_scissor_mode(glm::ivec2 pos, glm::ivec2 size);
  bool end_scissor_mode();

  // glm::vec2 glyph_size;
  float font_sample_point_size = 28.0f;
  float viewport_scale = 1.0f;

  float line_height;

  std::vector<uint8_t> _font_buffer; // must outlive _font_info
  stbtt_fontinfo _font_info;
  float _font_scale;
  std::unordered_map<int, GlyphMetrics> _glyph_metrics;
  const int _glyph_padding = 1;

private:
  // TODO: Having these sdl gpu types are bad since it exposes it to the end
  // user

  SDL_Window *_window;
  SDL_GPUDevice *_device;
  const char *_title;

  // GPU resource IDs
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

  TextureID _dummy_texture_id;
  TextureID _font_atlas_id;

  GeometryID _quad_geometry_id;

  GraphicsPipelineID _sdf_rect_stroke_pipeline_id;
  GraphicsPipelineID _sprite_pipeline_id;
  GraphicsPipelineID _text_pipeline_id;
  GraphicsPipelineID _film_pipeline_id;

  SDL_GPUSampleCount _sample_count = SDL_GPU_SAMPLECOUNT_1;

  const uint32_t _threadcount_x = 16;
  const uint32_t _threadcount_y = 16;
  const uint32_t _threadcount_z = 1;

  // TODO: Input file path sanitizing
  const std::string regular_font_path = "assets/fonts/MartianMono-Regular.ttf";
};
