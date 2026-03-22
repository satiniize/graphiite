#pragma once

#include <cstdint>
#include <string>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <glm/mat4x4.hpp>

#include "image.hpp"
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
  glm::vec4 stroke_thickness;
  uint32_t tiling;
  uint32_t use_texture;
  uint32_t draw_stroke;
};

struct RawProcessorFragmentUniformBuffer {
  glm::mat4 correction_matrix;
  glm::mat4 crosstalk_matrix;
  glm::mat4 dye_absorption_matrix;
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
};

struct RenderTargetParams {
  int width;
  int height;
  SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  int sample_count = 1;
};

// TODO: When renderer changes, sprite renderer and clay renderer needs to
// recompile
class Renderer {
public:
  uint32_t width;
  uint32_t height;

  Renderer(uint32_t width, uint32_t height);
  ~Renderer();
  // Texture ID is a key in gpu_textures for the cpu to handle easily.
  //
  TextureID create_render_target(int w, int h);
  TextureID upload_texture(const Image &image);
  GeometryID upload_geometry(const Vertex *vertices, size_t vertex_size,
                             const Uint16 *indices, size_t index_size);
  GraphicsPipelineID
  create_graphics_pipeline(SDL_GPUShader *vertex_shader,
                           SDL_GPUShader *fragment_shader,
                           bool is_non_swapchain_pipeline = false);
  // TODO: These aren't mature, redesign please
  TextureID load_and_upload_ascii_font_atlas(
      const std::string &font_path); // TODO: Seperation of concerns
  void create_render_targets();      // TODO: Shotgun
  // Loop functions
  bool update_swapchain_texture(); // TODO: Not much of a descriptive name
  bool begin_frame();
  bool end_frame();
  // Drawing functions
  void film_pass(RawProcessorFragmentUniformBuffer frag_uniforms);
  bool draw_sprite(TextureID texture_id, glm::vec2 translation, float rotation,
                   glm::vec2 scale, glm::vec4 color);
  bool draw_rect(RectParams params);
  bool draw_text(const char *text, int length, float point_size,
                 glm::vec2 position, glm::vec4 color);
  // Scissor mode
  bool begin_scissor_mode(glm::ivec2 pos, glm::ivec2 size);
  bool end_scissor_mode();

  glm::vec2 glyph_size;
  float font_sample_point_size = 56.0f;
  float viewport_scale = 1.0f;

  TextureID film_source_texture_id;
  TextureID film_render_target_id;

private:
  Context context;

  // GPU resource IDs
  TextureID next_texture_id = 0;
  GeometryID next_geometry_id = 0;
  GraphicsPipelineID next_pipeline_id = 0;
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

  TextureID dummy_texture_id;
  TextureID italic_font_atlas_id;
  TextureID regular_font_atlas_id;

  // TODO: Make this a public API so we don't have to do this

  GeometryID quad_geometry_id;

  GraphicsPipelineID sdf_rect_stroke_pipeline_id;
  GraphicsPipelineID sprite_pipeline_id;
  GraphicsPipelineID text_pipeline_id;
  GraphicsPipelineID film_pipeline_id;

  SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;

  // TODO: Input file path sanitizing
  const std::string regular_font_path =
      "assets/fonts/AtkinsonHyperlegibleMono-Medium.ttf";
  const std::string italic_font_path = "assets/fonts/XanhMono-Italic.ttf";
};
