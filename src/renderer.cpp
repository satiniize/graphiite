#include "renderer.hpp"
#include "SDL3/SDL_log.h"
#include "image.hpp"
#include "texture.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <cstddef>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

// Vertex uniform blocks
struct BasicVertexUniformBuffer {
  glm::mat4 mvp_matrix;
};

// Fragment uniform blocks
struct CommonFragmentUniformBuffer {
  float time;
};

struct SpriteFragmentUniformBuffer {
  glm::vec4 modulate;
};

std::vector<char> load_shader(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    SDL_Log("Failed to load shader from disk! %s", path.c_str());
    return {};
  }
  size_t code_size = file.tellg();
  std::vector<char> code(code_size);
  file.seekg(0);
  file.read(code.data(), code_size);
  return code;
}

Renderer::Renderer(std::string name, uint32_t width, uint32_t height) {
  this->width = width;
  this->height = height;
  this->_title = name.c_str();

  // SDL setup
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return;
  }
  SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

  // Create GPU device
  this->_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  if (this->_device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return;
  }

  // Create window
  SDL_WindowFlags flags =
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  this->_window = SDL_CreateWindow(this->_title, width, height, flags);
  if (!this->_window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    return;
  }

  // Claim window and GPU device
  if (!SDL_ClaimWindowForGPUDevice(this->_device, this->_window)) {
    SDL_Log("GPUClaimWindow failed");
    return;
  }

  SDL_GPUTextureFormat format =
      SDL_GetGPUSwapchainTextureFormat(this->_device, this->_window);

  if (!SDL_GPUTextureSupportsSampleCount(this->_device, format,
                                         this->_sample_count)) {
    SDL_Log("MSAA not supported");
    this->_sample_count = SDL_GPU_SAMPLECOUNT_1;
  }

  // Doesn't seem to work in wayland
  // SDL_SetGPUSwapchainParameters(this->_device, this->_window,
  //                               SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
  //                               SDL_GPU_PRESENTMODE_IMMEDIATE);

  std::vector<char> basic_vertex_shader_code =
      load_shader("assets/shaders/basic.vert.spv");
  std::vector<char> text_vertex_shader_code =
      load_shader("assets/shaders/text.vert.spv");

  std::vector<char> sprite_fragment_shader_code =
      load_shader("assets/shaders/sprite.frag.spv");
  std::vector<char> text_fragment_shader_code =
      load_shader("assets/shaders/text.frag.spv");
  std::vector<char> sdf_rect_stroke_fragment_shader_code =
      load_shader("assets/shaders/sdf_rect_stroke.frag.spv");
  std::vector<char> film_compute_shader_code =
      load_shader("assets/shaders/film_compute.comp.spv");

  _sprite_pipeline_id = create_graphics_pipeline(
      GraphicsPipelineParams{.num_vertex_uniform_buffers = 1,
                             .num_fragment_uniform_buffers = 2,
                             .num_fragment_samplers = 1},
      basic_vertex_shader_code, sprite_fragment_shader_code);
  _text_pipeline_id = create_graphics_pipeline(
      GraphicsPipelineParams{.num_vertex_uniform_buffers = 1,
                             .num_fragment_uniform_buffers = 2,
                             .num_fragment_samplers = 1},
      text_vertex_shader_code, text_fragment_shader_code);
  _sdf_rect_stroke_pipeline_id = create_graphics_pipeline(
      GraphicsPipelineParams{.num_vertex_uniform_buffers = 1,
                             .num_fragment_uniform_buffers = 2,
                             .num_fragment_samplers = 1},
      basic_vertex_shader_code, sdf_rect_stroke_fragment_shader_code);
  _film_pipeline_id = create_compute_pipeline(
      ComputePipelineParams{.num_samplers = 1,
                            .num_readwrite_storage_textures = 1,
                            .num_uniform_buffers = 1},
      film_compute_shader_code);

  // Create gpu sampler
  SDL_GPUSamplerCreateInfo clamp_sampler_info{};
  clamp_sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
  clamp_sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
  clamp_sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
  clamp_sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  clamp_sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  clamp_sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

  _clamp_sampler = SDL_CreateGPUSampler(this->_device, &clamp_sampler_info);
  if (!_clamp_sampler) {
    SDL_Log("Failed to create GPU sampler");
    return;
  }

  SDL_GPUSamplerCreateInfo wrap_sampler_info{};
  wrap_sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
  wrap_sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
  wrap_sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
  wrap_sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
  wrap_sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
  wrap_sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

  _wrap_sampler = SDL_CreateGPUSampler(this->_device, &wrap_sampler_info);
  if (!_wrap_sampler) {
    SDL_Log("Failed to create GPU sampler");
    return;
  }

  Vertex quad_vertices[]{
      {-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},  // top left
      {0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},   // top right
      {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // bottom left
      {0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}   // bottom right
  };

  Uint16 quad_indices[]{0, 1, 2, 2, 1, 3};

  _quad_geometry_id =
      upload_geometry(quad_vertices, std::size(quad_vertices) * sizeof(Vertex),
                      quad_indices, std::size(quad_indices) * sizeof(Uint16));

  _font_atlas = load_and_upload_ascii_font_atlas(regular_font_path);

  Image dummy_image;
  dummy_image.pixels = {0, 0, 0, 0};
  dummy_image.width = 1;
  dummy_image.height = 1;
  dummy_image.channels = 4;
  dummy_image.pixel_format = PixelFormat::RGBA8;
  _dummy_texture_id = upload_texture(dummy_image).id;

  create_render_targets();

  return;
}

Renderer::~Renderer() {
  for (auto &[path, graphics_pipeline] : _graphics_pipelines) {
    SDL_ReleaseGPUGraphicsPipeline(this->_device, graphics_pipeline);
  }

  for (auto &[path, texture] : _gpu_textures) {
    SDL_ReleaseGPUTexture(this->_device, texture);
  }

  if (_color_render_target)
    SDL_ReleaseGPUTexture(this->_device, _color_render_target);
  if (_resolve_target)
    SDL_ReleaseGPUTexture(this->_device, _resolve_target);

  for (auto &[name, buffer] : _vertex_buffers) {
    SDL_ReleaseGPUBuffer(this->_device, buffer);
  }

  for (auto &[name, buffer] : _index_buffers) {
    SDL_ReleaseGPUBuffer(this->_device, buffer);
  }

  SDL_ReleaseGPUSampler(this->_device, _clamp_sampler);
  SDL_ReleaseGPUSampler(this->_device, _wrap_sampler);

  SDL_ReleaseWindowFromGPUDevice(this->_device, this->_window);
  SDL_DestroyGPUDevice(this->_device);
  SDL_DestroyWindow(this->_window);

  SDL_Quit();

  return;
}

// TODO: redo this. this is only for film
Texture Renderer::create_compute_target_texture(int w, int h, bool is_sampler) {
  SDL_GPUTextureCreateInfo info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE |
               (is_sampler ? SDL_GPU_TEXTUREUSAGE_SAMPLER : 0),
      .width = static_cast<uint32_t>(w),
      .height = static_cast<uint32_t>(h),
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
  };

  SDL_GPUTexture *render_target = SDL_CreateGPUTexture(this->_device, &info);
  _gpu_textures[_next_texture_id] = render_target;
  // return _next_texture_id++;

  // _gpu_textures[_next_texture_id] = texture;

  Texture render_target_texture;
  render_target_texture.id = _next_texture_id++;
  render_target_texture.width = w;
  render_target_texture.height = h;
  render_target_texture.channels = 4;
  render_target_texture.pixel_format = PixelFormat::RGBA8;
  render_target_texture.tiling = false;

  return render_target_texture;
}

// TODO: return dummy texture on invalid image
Texture Renderer::upload_texture(const Image &image) {
  int bytes_per_pixel = image.bytes_per_pixel();
  size_t num_bytes =
      static_cast<size_t>(image.width * image.height * bytes_per_pixel);

  SDL_GPUTextureCreateInfo texture_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = image.pixel_format == PixelFormat::RGBA8
                    ? SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM
                    : SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = image.width,
      .height = image.height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
  };

  SDL_GPUTexture *texture = SDL_CreateGPUTexture(this->_device, &texture_info);
  if (!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create GPU texture: %s", SDL_GetError());
    return Texture();
  }

  SDL_GPUTransferBufferCreateInfo texture_transfer_create_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = static_cast<Uint32>(num_bytes),
  };
  SDL_GPUTransferBuffer *texture_transfer_buffer =
      SDL_CreateGPUTransferBuffer(this->_device, &texture_transfer_create_info);

  void *texture_data_ptr =
      SDL_MapGPUTransferBuffer(this->_device, texture_transfer_buffer, false);
  SDL_memcpy(texture_data_ptr, image.pixels.data(), num_bytes);

  SDL_UnmapGPUTransferBuffer(this->_device, texture_transfer_buffer);

  SDL_GPUCommandBuffer *_command_buffer =
      SDL_AcquireGPUCommandBuffer(this->_device); // GPU command buffer
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(_command_buffer);

  SDL_GPUTextureTransferInfo texture_transfer_info{
      .transfer_buffer = texture_transfer_buffer,
      .offset = 0,
      .pixels_per_row = image.width,
      .rows_per_layer = image.height,
  };
  SDL_GPUTextureRegion texture_region{
      .texture = texture,
      .mip_level = 0,
      .layer = 0,
      .x = 0,
      .y = 0,
      .z = 0,
      .w = image.width,
      .h = image.height,
      .d = 1,
  };
  SDL_UploadToGPUTexture(copyPass, &texture_transfer_info, &texture_region,
                         false);
  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(_command_buffer);
  SDL_ReleaseGPUTransferBuffer(this->_device, texture_transfer_buffer);

  _gpu_textures[_next_texture_id] = texture;

  Texture result;
  result.id = _next_texture_id++;
  result.width = image.width;
  result.height = image.height;
  result.channels = image.channels;
  result.pixel_format = image.pixel_format;
  result.tiling = false;

  return result;
}

Image Renderer::download_texture(Texture &texture) {
  SDL_GPUTexture *gpu_texture = _gpu_textures[texture.id];
  int bytes_per_pixel = texture.bytes_per_pixel();
  int num_bytes =
      static_cast<uint32_t>(texture.width * texture.height * bytes_per_pixel);

  // Set up transfer buffer
  SDL_GPUTransferBufferCreateInfo texture_transfer_create_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
      .size = static_cast<Uint32>(num_bytes),
  };
  SDL_GPUTransferBuffer *texture_transfer_buffer =
      SDL_CreateGPUTransferBuffer(this->_device, &texture_transfer_create_info);

  // Start a copy pass
  SDL_GPUCommandBuffer *_command_buffer =
      SDL_AcquireGPUCommandBuffer(this->_device);
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(_command_buffer);

  // Upload texture data to the GPU texture
  SDL_GPUTextureTransferInfo texture_transfer_info{
      .transfer_buffer = texture_transfer_buffer,
      .offset = 0,
      .pixels_per_row = texture.width,
      .rows_per_layer = texture.height,
  };
  SDL_GPUTextureRegion texture_region{
      .texture = gpu_texture,
      .mip_level = 0,
      .layer = 0,
      .x = 0,
      .y = 0,
      .z = 0,
      .w = texture.width,
      .h = texture.height,
      .d = 1,
  };

  SDL_DownloadFromGPUTexture(copyPass, &texture_region, &texture_transfer_info);

  SDL_EndGPUCopyPass(copyPass);
  SDL_GPUFence *fence =
      SDL_SubmitGPUCommandBufferAndAcquireFence(_command_buffer);
  SDL_WaitForGPUFences(this->_device, true, &fence, 1);
  SDL_ReleaseGPUFence(this->_device, fence);

  void *downloaded_data =
      SDL_MapGPUTransferBuffer(this->_device, texture_transfer_buffer, false);
  Image image(texture.width, texture.height, texture.pixel_format);
  SDL_memcpy(image.pixels.data(), downloaded_data, num_bytes);
  SDL_UnmapGPUTransferBuffer(this->_device, texture_transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(this->_device, texture_transfer_buffer);
  image.channels = 4;
  return image;
};

GeometryID Renderer::upload_geometry(const Vertex *vertices, size_t vertex_size,
                                     const Uint16 *indices, size_t index_size) {

  // Create the vertex buffer
  SDL_GPUBufferCreateInfo vertex_buffer_info{};
  vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertex_buffer_info.size = vertex_size;
  SDL_GPUBuffer *vertex_buffer =
      SDL_CreateGPUBuffer(this->_device, &vertex_buffer_info);
  if (!vertex_buffer) {
    SDL_Log("Failed to create vertex buffer");
    return false;
  }
  SDL_SetGPUBufferName(this->_device, vertex_buffer, "Vertex Buffer");

  // Create the index buffer
  SDL_GPUBufferCreateInfo index_buffer_info{};
  index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  index_buffer_info.size = index_size;
  SDL_GPUBuffer *index_buffer =
      SDL_CreateGPUBuffer(this->_device, &index_buffer_info);
  if (!index_buffer) {
    SDL_Log("Failed to create index buffer");
    return false;
  }
  SDL_SetGPUBufferName(this->_device, index_buffer, "Index Buffer");

  // Create a transfer buffer to upload to the vertex buffer
  SDL_GPUTransferBufferCreateInfo vertex_transfer_create_info{};
  vertex_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  vertex_transfer_create_info.size = vertex_size + index_size;
  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(this->_device, &vertex_transfer_create_info);
  if (!transfer_buffer) {
    SDL_Log("Failed to create transfer buffer");
    return false;
  }

  // Fill the transfer buffer
  Vertex *data =
      (Vertex *)SDL_MapGPUTransferBuffer(this->_device, transfer_buffer, false);
  SDL_memcpy(data, (void *)vertices, vertex_size);
  Uint16 *indexData = (Uint16 *)&data[(vertex_size / sizeof(Vertex))];
  SDL_memcpy(indexData, (void *)indices, index_size);
  // Unmap the pointer when you are done updating the transfer buffer
  SDL_UnmapGPUTransferBuffer(this->_device, transfer_buffer);

  // Start a copy pass
  SDL_GPUCommandBuffer *_command_buffer =
      SDL_AcquireGPUCommandBuffer(this->_device);

  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(_command_buffer);

  // Where is the data
  SDL_GPUTransferBufferLocation vertex_location{};
  vertex_location.transfer_buffer = transfer_buffer;
  vertex_location.offset = 0;

  // Where to upload the data
  SDL_GPUBufferRegion vertex_region{};
  vertex_region.buffer = vertex_buffer;
  vertex_region.size = vertex_size;
  vertex_region.offset = 0;

  // Upload the data
  SDL_UploadToGPUBuffer(copyPass, &vertex_location, &vertex_region, false);

  // Where is the data
  SDL_GPUTransferBufferLocation index_location{};
  index_location.transfer_buffer = transfer_buffer;
  index_location.offset = vertex_size;

  // Where to upload the data
  SDL_GPUBufferRegion index_region{};
  index_region.buffer = index_buffer;
  index_region.size = index_size;
  index_region.offset = 0;

  SDL_UploadToGPUBuffer(copyPass, &index_location, &index_region, false);

  // End copy pass
  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(_command_buffer);
  SDL_ReleaseGPUTransferBuffer(this->_device, transfer_buffer);

  _vertex_buffers[_next_geometry_id] = vertex_buffer;
  _index_buffers[_next_geometry_id] = index_buffer;

  return _next_geometry_id++;
};

// LGTM
GraphicsPipelineID
Renderer::create_graphics_pipeline(GraphicsPipelineParams params,
                                   std::vector<char> vertex_code,
                                   std::vector<char> fragment_code) {
  SDL_GPUShaderCreateInfo vertex_shader_info{};
  vertex_shader_info.code_size = vertex_code.size();
  vertex_shader_info.code = (Uint8 *)vertex_code.data();
  vertex_shader_info.entrypoint = "main";
  vertex_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
  vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
  vertex_shader_info.num_samplers = params.num_vertex_samplers;
  vertex_shader_info.num_storage_textures = params.num_vertex_storage_textures;
  vertex_shader_info.num_storage_buffers = params.num_vertex_storage_buffers;
  vertex_shader_info.num_uniform_buffers = params.num_vertex_uniform_buffers;

  SDL_GPUShader *vertex_shader =
      SDL_CreateGPUShader(this->_device, &vertex_shader_info);

  if (vertex_shader == NULL) {
    SDL_Log("Failed to create vertex shader!");
    return -1;
  }

  SDL_GPUShaderCreateInfo fragment_shader_info{};
  fragment_shader_info.code_size = fragment_code.size();
  fragment_shader_info.code =
      reinterpret_cast<const Uint8 *>(fragment_code.data());
  fragment_shader_info.entrypoint = "main";
  fragment_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
  fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  fragment_shader_info.num_samplers = params.num_fragment_samplers;
  fragment_shader_info.num_storage_textures =
      params.num_fragment_storage_textures;
  fragment_shader_info.num_storage_buffers =
      params.num_fragment_storage_buffers;
  fragment_shader_info.num_uniform_buffers =
      params.num_fragment_uniform_buffers;

  SDL_GPUShader *fragment_shader =
      SDL_CreateGPUShader(this->_device, &fragment_shader_info);

  if (fragment_shader == NULL) {
    SDL_Log("Failed to create fragment shader!");
    SDL_ReleaseGPUShader(this->_device, vertex_shader);
    return -1;
  }

  static const uint32_t num_vertex_buffers = 1;
  SDL_GPUVertexBufferDescription
      vertex_buffer_descriptions[num_vertex_buffers] = {};
  vertex_buffer_descriptions[0].slot = 0;
  vertex_buffer_descriptions[0].pitch = sizeof(Vertex);
  vertex_buffer_descriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  vertex_buffer_descriptions[0].instance_step_rate = 0;

  static const uint32_t num_vertex_attributes = 3;
  SDL_GPUVertexAttribute vertex_attributes[num_vertex_attributes] = {};
  // a_position
  vertex_attributes[0].location = 0;
  vertex_attributes[0].buffer_slot = 0;
  vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  vertex_attributes[0].offset = 0;
  // a_color
  vertex_attributes[1].location = 1;
  vertex_attributes[1].buffer_slot = 0;
  vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
  vertex_attributes[1].offset = sizeof(float) * 3;
  // a_texcoord
  vertex_attributes[2].location = 2;
  vertex_attributes[2].buffer_slot = 0;
  vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
  vertex_attributes[2].offset = sizeof(float) * 7;

  static const uint32_t num_color_targets = 1;

  SDL_GPUColorTargetDescription color_target_descriptions[num_color_targets];
  color_target_descriptions[0] = {};
  color_target_descriptions[0].format =
      SDL_GetGPUSwapchainTextureFormat(this->_device, this->_window);
  color_target_descriptions[0].blend_state.src_color_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE;
  color_target_descriptions[0].blend_state.dst_color_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  color_target_descriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
  color_target_descriptions[0].blend_state.src_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE;
  color_target_descriptions[0].blend_state.dst_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  color_target_descriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
  color_target_descriptions[0].blend_state.enable_blend = true;

  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
      .vertex_shader = vertex_shader,
      .fragment_shader = fragment_shader,
      .vertex_input_state =
          SDL_GPUVertexInputState{
              .vertex_buffer_descriptions = vertex_buffer_descriptions,
              .num_vertex_buffers = num_vertex_buffers,
              .vertex_attributes = vertex_attributes,
              .num_vertex_attributes = num_vertex_attributes,
          },
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .rasterizer_state = {},
      .multisample_state =
          {
              .sample_count = this->_sample_count,
              .sample_mask = 0,
              .enable_mask = false,
              .enable_alpha_to_coverage = false,
          },
      .depth_stencil_state = {},
      .target_info =
          {
              .color_target_descriptions = color_target_descriptions,
              .num_color_targets = num_color_targets,
          },
      .props = 0,
  };

  SDL_GPUGraphicsPipeline *graphics_pipeline =
      SDL_CreateGPUGraphicsPipeline(this->_device, &pipeline_info);

  SDL_ReleaseGPUShader(this->_device, vertex_shader);
  SDL_ReleaseGPUShader(this->_device, fragment_shader);

  if (!graphics_pipeline) {
    SDL_Log("Failed to create graphics pipeline");
    return -1;
  }

  _graphics_pipelines[_next_graphics_pipeline_id] = graphics_pipeline;

  return _next_graphics_pipeline_id++;
}

ComputePipelineID
Renderer::create_compute_pipeline(ComputePipelineParams params,
                                  std::vector<char> compute_code) {
  SDL_GPUComputePipelineCreateInfo pipeline_info = {
      .code_size = compute_code.size(),
      .code = reinterpret_cast<const Uint8 *>(compute_code.data()),
      .entrypoint = "main",
      .format = SDL_GPU_SHADERFORMAT_SPIRV,
      .num_samplers = params.num_samplers,
      .num_readonly_storage_textures = params.num_readonly_storage_textures,
      .num_readonly_storage_buffers = params.num_readonly_storage_buffers,
      .num_readwrite_storage_textures = params.num_readwrite_storage_textures,
      .num_readwrite_storage_buffers = params.num_readwrite_storage_buffers,
      .num_uniform_buffers = params.num_uniform_buffers,
      .threadcount_x = _threadcount_x,
      .threadcount_y = _threadcount_y,
      .threadcount_z = _threadcount_z,
      .props = 0,
  };

  SDL_GPUComputePipeline *compute_pipeline =
      SDL_CreateGPUComputePipeline(this->_device, &pipeline_info);

  if (!compute_pipeline) {
    SDL_Log("Failed to create compute pipeline");
    return -1;
  }

  _compute_pipelines[_next_compute_pipeline_id] = compute_pipeline;

  return _next_compute_pipeline_id++;
}

Texture
Renderer::load_and_upload_ascii_font_atlas(const std::string &font_path) {
  // Load font file
  std::ifstream font_file(font_path, std::ios::binary | std::ios::ate);
  std::streampos file_size = font_file.tellg();
  font_file.seekg(0, std::ios::beg);

  std::vector<uint8_t> _font_buffer; // must outlive _font_info
  _font_buffer.resize(static_cast<size_t>(file_size));
  font_file.read(reinterpret_cast<char *>(_font_buffer.data()), file_size);

  stbtt_fontinfo _font_info;

  stbtt_InitFont(&_font_info, _font_buffer.data(), 0);
  stbtt_GetFontVMetrics(&_font_info, &font_ascent, &font_descent,
                        &font_line_gap);
  _font_scale = stbtt_ScaleForPixelHeight(&_font_info, font_sample_point_size);

  // Collect rects for stb_rect_pack
  // We expand each glyph by _glyph_padding on all sides, so two adjacent
  // glyphs have _glyph_padding + _glyph_padding pixels between their ink
  constexpr int FIRST_CODEPOINT = 32;
  constexpr int LAST_CODEPOINT = 126;
  constexpr int NUM_GLYPHS = LAST_CODEPOINT - FIRST_CODEPOINT + 1;
  constexpr int ATLAS_SIZE = 1024;

  struct GlyphBitmap {
    int codepoint;
    int bw, bh;      // bitmap dimensions (without padding)
    int xoff, yoff;  // bearing from stbtt
    uint8_t *pixels; // owned, freed after upload
  };

  std::vector<GlyphBitmap> bitmaps;
  bitmaps.reserve(NUM_GLYPHS);

  std::vector<stbrp_rect> pack_rects;
  pack_rects.reserve(NUM_GLYPHS);

  for (int cp = FIRST_CODEPOINT; cp <= LAST_CODEPOINT; cp++) {
    GlyphBitmap gb{};
    gb.codepoint = cp;

    if (cp == 32) {
      // Space: no bitmap, but we still need metrics
      gb.pixels = nullptr;
      gb.bw = 0;
      gb.bh = 0;
      gb.xoff = 0;
      gb.yoff = 0;
    } else {
      // SDF swap point: replace stbtt_GetCodepointBitmap with
      // stbtt_GetCodepointSDF for signed distance field rendering
      gb.pixels = stbtt_GetCodepointBitmap(&_font_info, 0, _font_scale, cp,
                                           &gb.bw, &gb.bh, &gb.xoff, &gb.yoff);
    }

    bitmaps.push_back(gb);

    stbrp_rect r{};
    r.id = cp;
    r.w = gb.bw + _glyph_padding * 2;
    r.h = gb.bh + _glyph_padding * 2;
    pack_rects.push_back(r);
  }

  // Pack rects
  stbrp_context pack_ctx{};
  std::vector<stbrp_node> pack_nodes(ATLAS_SIZE);
  stbrp_init_target(&pack_ctx, ATLAS_SIZE, ATLAS_SIZE, pack_nodes.data(),
                    ATLAS_SIZE);
  stbrp_pack_rects(&pack_ctx, pack_rects.data(),
                   static_cast<int>(pack_rects.size()));

  // Build RGBA atlas
  Image atlas{};
  atlas.width = ATLAS_SIZE;
  atlas.height = ATLAS_SIZE;
  atlas.channels = 4;
  atlas.pixel_format = PixelFormat::RGBA8;
  atlas.pixels.resize(ATLAS_SIZE * ATLAS_SIZE * 4, 0);

  for (int i = 0; i < NUM_GLYPHS; i++) {
    const GlyphBitmap &gb = bitmaps[i];
    const stbrp_rect &r = pack_rects[i];

    // Resolve advance and bearing for all glyphs including space
    int adv_raw, lsb_raw;
    stbtt_GetCodepointHMetrics(&_font_info, gb.codepoint, &adv_raw, &lsb_raw);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&_font_info, &ascent, &descent, &line_gap);
    int ascent_px = static_cast<int>(roundf(ascent * _font_scale));
    int descent_px = static_cast<int>(roundf(descent * _font_scale));
    int glyph_height = ascent_px - descent_px;

    GlyphMetrics gm{};
    gm.size = glm::vec2(gb.bw, gb.bh);
    gm.bearing = glm::vec2(gb.xoff, gb.yoff);
    gm.advance = roundf(adv_raw * _font_scale);

    if (r.was_packed && gb.bw > 0 && gb.bh > 0) {
      // Ink starts at (r.x + padding, r.y + padding)
      int ink_x = r.x + _glyph_padding;
      int ink_y = r.y + _glyph_padding;

      // Blit grayscale bitmap into RGBA atlas as white + alpha
      for (int gy = 0; gy < gb.bh; gy++) {
        for (int gx = 0; gx < gb.bw; gx++) {
          int ax = ink_x + gx;
          int ay = ink_y + gy;
          if (ax < 0 || ax >= ATLAS_SIZE || ay < 0 || ay >= ATLAS_SIZE)
            continue;
          int idx = (ay * ATLAS_SIZE + ax) * 4;
          atlas.pixels[idx + 0] = 255;
          atlas.pixels[idx + 1] = 255;
          atlas.pixels[idx + 2] = 255;
          atlas.pixels[idx + 3] = gb.pixels[gy * gb.bw + gx];
        }
      }

      // UV rect covers only the ink region (excluding padding)
      gm.uv_rect = glm::vec4(static_cast<float>(ink_x) / ATLAS_SIZE,
                             static_cast<float>(ink_y) / ATLAS_SIZE,
                             static_cast<float>(ink_x + gb.bw) / ATLAS_SIZE,
                             static_cast<float>(ink_y + gb.bh) / ATLAS_SIZE);
    } else {
      gm.uv_rect = glm::vec4(0.0f);
    }

    _glyph_metrics[gb.codepoint] = gm;

    // Store line height on the renderer using ascent/descent
    // We use the font's line height at sample point size as reference
    // draw_text scales this by (point_size / font_sample_point_size)
    // glyph_size.y is repurposed here to store line height only
    line_height = static_cast<float>(glyph_height);

    if (gb.pixels) {
      stbtt_FreeBitmap(gb.pixels, nullptr);
    }
  }

  return this->upload_texture(atlas);
}

void Renderer::create_render_targets() {
  SDL_GPUTextureFormat swapchain_format =
      SDL_GetGPUSwapchainTextureFormat(this->_device, this->_window);
  SDL_GPUTextureCreateInfo msaa_render_target_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = swapchain_format,
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
      .width = this->width,
      .height = this->height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = _sample_count};

  if (_sample_count == SDL_GPU_SAMPLECOUNT_1) {
    msaa_render_target_info.usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
  }

  _color_render_target =
      SDL_CreateGPUTexture(this->_device, &msaa_render_target_info);

  if (_sample_count == SDL_GPU_SAMPLECOUNT_1) {
    _resolve_target = nullptr;
    return;
  }

  SDL_GPUTextureCreateInfo resolve_target_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = swapchain_format,
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = this->width,
      .height = this->height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1};

  _resolve_target = SDL_CreateGPUTexture(this->_device, &resolve_target_info);
}

void Renderer::start_frame() {
  _command_buffer = SDL_AcquireGPUCommandBuffer(this->_device);

  if (!_command_buffer) {
    SDL_Log("Failed to acquire GPU command buffer");
    return;
  }

  uint32_t new_width, new_height;
  SDL_WaitAndAcquireGPUSwapchainTexture(_command_buffer, this->_window,
                                        &this->_swapchain_texture, &new_width,
                                        &new_height);

  if (new_width == 0 || new_height == 0) {
    SDL_CancelGPUCommandBuffer(_command_buffer);
    return;
  }

  if (!this->_swapchain_texture) {
    SDL_CancelGPUCommandBuffer(_command_buffer);
    return;
  }

  // Resize render targets if necessary
  if (new_width != this->width || new_height != this->height) {
    if (_color_render_target)
      SDL_ReleaseGPUTexture(this->_device, _color_render_target);
    if (_resolve_target)
      SDL_ReleaseGPUTexture(this->_device, _resolve_target);
    this->width = new_width;
    this->height = new_height;
    create_render_targets();
  } else {
    this->width = new_width;
    this->height = new_height;
  }
};

void Renderer::end_frame() { SDL_SubmitGPUCommandBuffer(_command_buffer); };

bool Renderer::begin_compute_pass(Texture film_target_texture) {
  // _command_buffer = SDL_AcquireGPUCommandBuffer(this->_device);

  SDL_GPUStorageTextureReadWriteBinding binding = {
      .texture = _gpu_textures[film_target_texture.id], // TODO: store this like
                                                        // _commanf_buffer
  };
  _compute_pass =
      SDL_BeginGPUComputePass(_command_buffer, &binding, 1, NULL, 0);
  return true;
}

bool Renderer::end_compute_pass() {
  SDL_EndGPUComputePass(_compute_pass);
  // SDL_SubmitGPUCommandBuffer(_command_buffer);
  return true;
}

bool Renderer::compute_film(void *fragment_uniforms_ptr,
                            size_t fragment_uniforms_size,
                            Texture film_source_texture,
                            Texture film_target_texture) {
  if (film_source_texture.id == -1 || film_target_texture.id == -1)
    return false;

  SDL_GPUTextureSamplerBinding sampler_bindings[1];
  sampler_bindings[0] = SDL_GPUTextureSamplerBinding{
      .texture = _gpu_textures[film_source_texture.id],
      .sampler = _clamp_sampler, // or linear if you want interpolation
  };

  SDL_BindGPUComputePipeline(_compute_pass,
                             _compute_pipelines[_film_pipeline_id]);
  SDL_BindGPUComputeSamplers(_compute_pass, 0, sampler_bindings, 1);
  // Push params via uniform buffer
  SDL_PushGPUComputeUniformData(_command_buffer, 0, fragment_uniforms_ptr,
                                fragment_uniforms_size);

  // Ceil divide to cover every pixel
  uint32_t gx = (film_target_texture.width + 7) / 8;
  uint32_t gy = (film_target_texture.height + 7) / 8;
  SDL_DispatchGPUCompute(_compute_pass, gx, gy, 1);

  return true;
}

// TODO: Why was this seperated again?
bool Renderer::begin_render_pass() {

  SDL_GPUColorTargetInfo color_target_info{};
  // color_target_info.texture = this->_swapchain_texture;
  color_target_info.texture = this->_color_render_target;
  color_target_info.clear_color = SDL_FColor{1.0f, 0.0f, 1.0f, 1.0f};
  color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
  // color_target_info.store_op = SDL_GPU_STOREOP_STORE;
  if (this->_sample_count == SDL_GPU_SAMPLECOUNT_1) {
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    color_target_info.resolve_texture = NULL;
  } else {
    color_target_info.store_op = SDL_GPU_STOREOP_RESOLVE;
    color_target_info.resolve_texture = this->_resolve_target;
  }

  _render_pass =
      SDL_BeginGPURenderPass(_command_buffer, &color_target_info, 1, NULL);

  if (!_render_pass) {
    SDL_Log("Failed to begin GPU render pass");
    return false;
  }

  this->_projection_matrix =
      glm::ortho(0.0f, (float)this->width / viewport_scale,
                 -(float)this->height / viewport_scale, 0.0f);

  CommonFragmentUniformBuffer common_fragment_uniform_buffer{};
  common_fragment_uniform_buffer.time = SDL_GetTicksNS() / 1e9f;

  SDL_PushGPUFragmentUniformData(_command_buffer, 0,
                                 &common_fragment_uniform_buffer,
                                 sizeof(CommonFragmentUniformBuffer));

  return true;
}

bool Renderer::end_render_pass() {
  SDL_EndGPURenderPass(_render_pass);

  SDL_GPUTexture *blitSourceTexture =
      (this->_sample_count == SDL_GPU_SAMPLECOUNT_1)
          ? this->_color_render_target
          : this->_resolve_target;

  SDL_GPUBlitInfo blit_info = {
      .source = {.texture = blitSourceTexture,
                 .w = this->width,
                 .h = this->height},
      .destination = {.texture = _swapchain_texture, .w = width, .h = height},
      .load_op = SDL_GPU_LOADOP_DONT_CARE,
      .filter = SDL_GPU_FILTER_LINEAR};

  SDL_BlitGPUTexture(_command_buffer, &blit_info);

  // SDL_SubmitGPUCommandBuffer(_command_buffer);

  // TODO: Why is this at end frame?
  this->viewport_scale = SDL_GetWindowPixelDensity(this->_window);

  return true;
}

// LGTM
bool Renderer::draw_sprite(TextureID texture_id, glm::vec2 translation,
                           float rotation, glm::vec2 scale, glm::vec4 color) {
  // Vertex buffer
  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = _vertex_buffers[_quad_geometry_id];
  vertex_buffer_bindings[0].offset = 0;
  // Index buffer
  SDL_GPUBufferBinding index_buffer_bindings{
      .buffer = _index_buffers[_quad_geometry_id],
      .offset = 0,
  };
  // Samplers
  if (_gpu_textures.find(texture_id) == _gpu_textures.end()) {
    // TODO: Maybe bind a dummy texture instead
    SDL_Log("Texture not found in draw_sprite. Ignoring..");
  }
  SDL_GPUTextureSamplerBinding fragment_sampler_bindings[1];
  fragment_sampler_bindings[0].texture = _gpu_textures[texture_id];
  fragment_sampler_bindings[0].sampler = _clamp_sampler;
  // Uniforms
  SpriteFragmentUniformBuffer fragment_uniforms = {
      .modulate = color,
  };
  glm::mat4 model_matrix = glm::mat4(1.0f);
  model_matrix = glm::translate(model_matrix,
                                glm::vec3(translation.x, -translation.y, 0.0f));
  model_matrix = glm::rotate(model_matrix, glm::radians(rotation),
                             glm::vec3(0.0f, 0.0f, 1.0f));
  model_matrix = glm::scale(model_matrix, glm::vec3(scale, 1.0f));
  BasicVertexUniformBuffer vertex_uniforms{
      .mvp_matrix = this->_projection_matrix * model_matrix};

  SDL_BindGPUGraphicsPipeline(_render_pass,
                              _graphics_pipelines[_sprite_pipeline_id]);
  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);
  SDL_BindGPUIndexBuffer(_render_pass, &index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);
  SDL_BindGPUFragmentSamplers(_render_pass, 0, fragment_sampler_bindings, 1);
  SDL_PushGPUVertexUniformData(_command_buffer, 0, &vertex_uniforms,
                               sizeof(BasicVertexUniformBuffer));
  SDL_PushGPUFragmentUniformData(_command_buffer, 1, &fragment_uniforms,
                                 sizeof(SpriteFragmentUniformBuffer));
  SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0, 0);

  return true;
}

// LGTM
bool Renderer::draw_rect(RectParams params) {
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
  // Bind vertex buffer
  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = _vertex_buffers[_quad_geometry_id];
  vertex_buffer_bindings[0].offset = 0;
  // Bind index buffer
  SDL_GPUBufferBinding index_buffer_bindings{
      .buffer = _index_buffers[_quad_geometry_id],
      .offset = 0,
  };
  // Samplers
  if (params.use_texture &&
      _gpu_textures.find(params.texture_id) == _gpu_textures.end()) {
    // TODO: Maybe bind a dummy texture instead
    SDL_Log("Texture not found in draw_rect. Ignoring..");
  }
  SDL_GPUTextureSamplerBinding fragment_sampler_bindings[1];
  fragment_sampler_bindings[0].texture = params.use_texture
                                             ? _gpu_textures[params.texture_id]
                                             : _gpu_textures[_dummy_texture_id];
  fragment_sampler_bindings[0].sampler =
      params.tiling ? _wrap_sampler : _clamp_sampler;
  // Uniforms
  glm::vec4 inner_corner_radii =
      params.corner_radii -
      glm::vec4(std::min(params.stroke_thickness.z, params.stroke_thickness.x),
                std::min(params.stroke_thickness.z, params.stroke_thickness.y),
                std::min(params.stroke_thickness.w, params.stroke_thickness.x),
                std::min(params.stroke_thickness.w, params.stroke_thickness.y));
  glm::vec2 padded_size = params.size + glm::vec2(params.smoothing * 2.0f);
  SDFRectStrokeFragmentUniformBuffer fragment_uniforms{
      .size =
          glm::vec4(params.size.x, params.size.y, padded_size.x, padded_size.y),
      .modulate = params.color,
      .corner_radii = glm::vec4(params.corner_radii),
      .inner_corner_radii = inner_corner_radii,
      .stroke_thickness = params.stroke_thickness,
      .tiling = static_cast<uint32_t>(params.tiling ? 1 : 0),
      .use_texture = static_cast<uint32_t>(params.use_texture ? 1 : 0),
      .draw_stroke = static_cast<uint32_t>(params.draw_stroke ? 1 : 0),
      .smoothing = params.smoothing,
  };
  glm::mat4 model_matrix = glm::mat4(1.0f);
  model_matrix = glm::translate(
      model_matrix,
      glm::vec3(params.position.x + params.size.x / 2.0f,
                -(params.position.y + params.size.y / 2.0f), 0.0f));
  model_matrix = glm::scale(
      model_matrix,
      glm::vec3(params.size + glm::vec2(params.smoothing * 2.0f), 1.0f));
  BasicVertexUniformBuffer vertex_uniforms{
      .mvp_matrix = this->_projection_matrix * model_matrix,
  };

  SDL_BindGPUGraphicsPipeline(
      _render_pass, _graphics_pipelines[_sdf_rect_stroke_pipeline_id]);
  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);
  SDL_BindGPUIndexBuffer(_render_pass, &index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);
  SDL_BindGPUFragmentSamplers(_render_pass, 0, fragment_sampler_bindings, 1);
  SDL_PushGPUVertexUniformData(_command_buffer, 0, &vertex_uniforms,
                               sizeof(BasicVertexUniformBuffer));
  SDL_PushGPUFragmentUniformData(_command_buffer, 1, &fragment_uniforms,
                                 sizeof(SDFRectStrokeFragmentUniformBuffer));
  SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0, 0);

  return true;
}

// TODO: Implement batch rendering/instancing using storage buffers?
bool Renderer::draw_text(const char *text, int length, float point_size,
                         glm::vec2 position, glm::vec4 color) {
  struct TextVertexUniformBuffer {
    glm::mat4 mvp_matrix;
    float time;
    float offset;
    float padding1;
    float padding2;
  };

  struct TextFragmentUniformBuffer {
    glm::vec4 modulate;
    glm::vec4 uv_rect;
  };

  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = _vertex_buffers[_quad_geometry_id];
  vertex_buffer_bindings[0].offset = 0;

  SDL_GPUBufferBinding index_buffer_bindings[1];
  index_buffer_bindings[0].buffer = _index_buffers[_quad_geometry_id];
  index_buffer_bindings[0].offset = 0;

  if (_gpu_textures.find(_font_atlas.id) == _gpu_textures.end()) {
    SDL_Log("Font atlas not loaded");
    SDL_Quit();
    return false;
  }

  SDL_GPUTextureSamplerBinding fragment_sampler_bindings{};
  fragment_sampler_bindings.texture = _gpu_textures[_font_atlas.id];
  fragment_sampler_bindings.sampler = _clamp_sampler;

  SDL_BindGPUGraphicsPipeline(_render_pass,
                              _graphics_pipelines[_text_pipeline_id]);
  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);
  SDL_BindGPUIndexBuffer(_render_pass, index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);
  SDL_BindGPUFragmentSamplers(_render_pass, 0, &fragment_sampler_bindings, 1);

  float scalar = point_size / font_sample_point_size;

  int ascent, descent, line_gap;
  ascent = font_ascent;
  descent = font_descent;
  line_gap = font_line_gap;
  // stbtt_GetFontVMetrics(&_font_info, &ascent, &descent, &line_gap);
  int ascent_px = static_cast<int>(roundf(ascent * _font_scale));

  float cursor_x = position.x;

  for (int i = 0; i < length; i++) {
    int cp = static_cast<unsigned char>(text[i]);

    auto it = _glyph_metrics.find(cp);
    if (it == _glyph_metrics.end())
      continue;

    const GlyphMetrics &gm = it->second;

    // Space and zero-size glyphs: advance cursor, skip draw
    if (gm.size.x <= 0 || gm.size.y <= 0) {
      cursor_x += gm.advance * scalar;
      continue;
    }

    // bearing.y is negative-upward from stbtt (e.g. -12 means 12px above
    // baseline) We want to offset the glyph down from the baseline origin
    float glyph_x = cursor_x + gm.bearing.x * scalar;
    float glyph_y = position.y + (ascent_px + gm.bearing.y) * scalar;

    float glyph_w = ceilf(gm.size.x * scalar);
    float glyph_h = ceilf(gm.size.y * scalar);

    TextFragmentUniformBuffer text_fragment_uniform_buffer{};
    text_fragment_uniform_buffer.modulate = color;
    text_fragment_uniform_buffer.uv_rect = gm.uv_rect;

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix =
        glm::translate(model_matrix, glm::vec3(glyph_x, -glyph_y, 0.0f));
    model_matrix = glm::scale(model_matrix, glm::vec3(glyph_w, glyph_h, 1.0f));
    // Pivot: quad is assumed to be centered at origin, shift to top-left
    model_matrix = glm::translate(model_matrix, glm::vec3(0.5f, -0.5f, 0.0f));

    glm::mat4 view_matrix = glm::mat4(1.0f);
    TextVertexUniformBuffer text_vertex_uniform_buffer{};
    text_vertex_uniform_buffer.mvp_matrix =
        _projection_matrix * view_matrix * model_matrix;
    text_vertex_uniform_buffer.time = SDL_GetTicksNS() / 1e9f;
    text_vertex_uniform_buffer.offset = static_cast<float>(i);

    SDL_PushGPUFragmentUniformData(_command_buffer, 1,
                                   &text_fragment_uniform_buffer,
                                   sizeof(TextFragmentUniformBuffer));
    SDL_PushGPUVertexUniformData(_command_buffer, 0,
                                 &text_vertex_uniform_buffer,
                                 sizeof(TextVertexUniformBuffer));
    SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0, 0);

    cursor_x += gm.advance * scalar;
  }

  return true;
}

// LGTM
bool Renderer::begin_scissor_mode(glm::ivec2 pos, glm::ivec2 size) {
  const SDL_Rect rect = {
      pos.x,
      pos.y,
      size.x,
      size.y,
  };
  SDL_SetGPUScissor(_render_pass, &rect);
  return true;
}

// LGTM
bool Renderer::end_scissor_mode() {
  const SDL_Rect rect = {
      0,
      0,
      static_cast<int>(this->width),
      static_cast<int>(this->height),
  };
  SDL_SetGPUScissor(_render_pass, &rect);
  return true;
}

void Renderer::get_font_metrics(int *ascent, int *descent, int *line_gap) {
  *ascent = font_ascent;
  *descent = font_descent;
  *line_gap = font_line_gap;
};
