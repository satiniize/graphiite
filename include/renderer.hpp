#pragma once

#define WIDTH 320
#define HEIGHT 180

#include <iterator>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Context {
  SDL_Window *window;
  SDL_GPUDevice *device;
  const char *title;
};

// the vertex input layout
struct Vertex {
  float x, y, z;    // vec3 position
  float r, g, b, a; // vec4 color
  float u, v;       // vec2 texture coordinates
};

const int scale = 4;

// a list of vertices
static Vertex vertices[]{
    {-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}, // top left vertex
    {0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f},  // top right vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
     1.0f}, // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
    // bottom right vertex
};

static uint16_t indices[]{0, 1, 2, 2, 1, 3};

struct FragmentUniformBuffer {
  float time;
};

static FragmentUniformBuffer fragment_uniform_buffer{};

struct VertexUniformBuffer {
  glm::mat4 mvp_matrix;
};

static VertexUniformBuffer vertex_uniform_buffer{};

class Renderer {
public:
  Renderer();
  ~Renderer();
  bool init();
  bool begin_frame(); // Init things
  // Collect draw calls here
  bool end_frame(); // Submit draw calls and clean up
  bool loop();
  bool cleanup();

private:
  Context context;

  SDL_GPUGraphicsPipeline *graphics_pipeline;

  SDL_GPUBuffer *vertex_buffer;
  SDL_GPUBuffer *index_buffer;

  // std::vector<SDL_GPUTexture *> gpu_textures;

  SDL_GPUTexture *quad_texture;
  SDL_GPUSampler *quad_sampler;

  SDL_GPUTransferBuffer *transfer_buffer;
};
