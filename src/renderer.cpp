#include "renderer.hpp"

#include "SDL3/SDL.h"
#include "SDL3/SDL_gpu.h"
#include "glm/gtc/matrix_transform.hpp"
#include <fstream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// Helpers
SDL_GPUShader *load_shader(SDL_GPUDevice *device, std::string path,
                           int num_samplers, int num_storage_textures,
                           int num_storage_buffers, int num_uniform_buffers) {
  // Load the shader code
  size_t code_size;
  void *code = SDL_LoadFile(path.c_str(), &code_size);
  if (code == NULL) {
    SDL_Log("Failed to load shader from disk! %s", path.c_str());
    return NULL;
  }
  // Determine shader stage
  SDL_GPUShaderStage stage = SDL_GPU_SHADERSTAGE_VERTEX;
  if (SDL_strstr(path.c_str(), ".vert")) {
    stage = SDL_GPU_SHADERSTAGE_VERTEX;
  } else if (SDL_strstr(path.c_str(), ".frag")) {
    stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  } else {
    SDL_Log("Invalid shader stage!");
    return NULL;
  }
  // Create the shader
  SDL_GPUShaderCreateInfo vertex_info{};
  vertex_info.code_size = code_size;
  vertex_info.code = (Uint8 *)code;
  vertex_info.entrypoint = "main";                 // Most likely
  vertex_info.format = SDL_GPU_SHADERFORMAT_SPIRV; // For now
  vertex_info.stage = stage;
  vertex_info.num_samplers = num_samplers;
  vertex_info.num_storage_textures = num_storage_textures;
  vertex_info.num_storage_buffers = num_storage_buffers;
  vertex_info.num_uniform_buffers = num_uniform_buffers;

  SDL_GPUShader *shader = SDL_CreateGPUShader(device, &vertex_info);

  // Free the file because we don't need it
  SDL_free(code);

  if (shader == NULL) {
    SDL_Log("Failed to create shader!");
    return NULL;
  }

  return shader;
}

Renderer::Renderer(uint32_t width, uint32_t height) {
  this->width = width;
  this->height = height;

  this->context = Context{};
  this->context.title = "Software Renderer";

  // SDL setup
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return;
  }
  SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

  // Create GPU device
  this->context.device =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  if (this->context.device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return;
  }

  // Create window
  SDL_WindowFlags flags =
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  this->context.window =
      SDL_CreateWindow(this->context.title, width, height, flags);
  if (!this->context.window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    return;
  }

  // Claim window and GPU device
  if (!SDL_ClaimWindowForGPUDevice(this->context.device,
                                   this->context.window)) {
    SDL_Log("GPUClaimWindow failed");
    return;
  }

  // Disable V Sync for FPS testing
  // Doesn't seem to work in wayland
  // SDL_SetGPUSwapchainParameters(this->context.device, this->context.window,
  //                               SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
  //                               SDL_GPU_PRESENTMODE_IMMEDIATE);

  // Create shaders
  // TODO: Make this easier, read the file and see how many is needed
  // Basic vertex shader
  SDL_GPUShader *basic_vertex_shader = load_shader(
      this->context.device, "src/shaders/basic.vert.spv", 0, 0, 0, 1);

  // Text vertex shader
  SDL_GPUShader *text_vertex_shader = load_shader(
      this->context.device, "src/shaders/text.vert.spv", 0, 0, 0, 1);

  // Sprite fragment shader
  SDL_GPUShader *sprite_fragment_shader = load_shader(
      this->context.device, "src/shaders/sprite.frag.spv", 1, 0, 0, 1);

  // ColorRect fragment shader
  SDL_GPUShader *color_rect_fragment_shader = load_shader(
      this->context.device, "src/shaders/color_rect.frag.spv", 0, 0, 0, 1);

  // TextureRect fragment shader
  SDL_GPUShader *texture_rect_fragment_shader = load_shader(
      this->context.device, "src/shaders/texture_rect.frag.spv", 1, 0, 0, 1);

  // Text fragment shader
  SDL_GPUShader *text_fragment_shader = load_shader(
      this->context.device, "src/shaders/text.frag.spv", 1, 0, 0, 1);

  // Arc fragment shader
  SDL_GPUShader *arc_fragment_shader =
      load_shader(this->context.device, "src/shaders/arc.frag.spv", 0, 0, 0, 1);

  create_graphics_pipeline("SPRITE", basic_vertex_shader,
                           sprite_fragment_shader);
  create_graphics_pipeline("COLOR_RECT", basic_vertex_shader,
                           color_rect_fragment_shader);
  create_graphics_pipeline("TEXTURE_RECT", basic_vertex_shader,
                           texture_rect_fragment_shader);
  create_graphics_pipeline("TEXT", text_vertex_shader, text_fragment_shader);
  create_graphics_pipeline("ARC", basic_vertex_shader, arc_fragment_shader);

  // We don't need to store the shaders after creating the pipeline
  SDL_ReleaseGPUShader(context.device, basic_vertex_shader);
  SDL_ReleaseGPUShader(context.device, text_vertex_shader);
  SDL_ReleaseGPUShader(context.device, sprite_fragment_shader);
  SDL_ReleaseGPUShader(context.device, color_rect_fragment_shader);
  SDL_ReleaseGPUShader(context.device, texture_rect_fragment_shader);
  SDL_ReleaseGPUShader(context.device, text_fragment_shader);
  SDL_ReleaseGPUShader(context.device, arc_fragment_shader);

  // Create gpu sampler
  SDL_GPUSamplerCreateInfo clamp_sampler_info{};
  clamp_sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
  clamp_sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
  clamp_sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
  clamp_sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  clamp_sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  clamp_sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

  clamp_sampler = SDL_CreateGPUSampler(context.device, &clamp_sampler_info);
  if (!clamp_sampler) {
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

  wrap_sampler = SDL_CreateGPUSampler(context.device, &wrap_sampler_info);
  if (!wrap_sampler) {
    SDL_Log("Failed to create GPU sampler");
    return;
  }

  static Vertex quad_vertices[]{
      {-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},  // top left
      {0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},   // top right
      {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // bottom left
      {0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}   // bottom right
  };

  static Uint16 quad_indices[]{0, 1, 2, 2, 1, 3};

  load_geometry("QUAD", quad_vertices,
                std::size(quad_vertices) * sizeof(Vertex), quad_indices,
                std::size(quad_indices) * sizeof(Uint16));

  std::ifstream fontFile(default_font_path, std::ios::binary | std::ios::ate);
  std::streamsize size = fontFile.tellg();
  fontFile.seekg(0, std::ios::beg);

  std::vector<uint8_t> font_buffer(size);
  fontFile.read(reinterpret_cast<char *>(font_buffer.data()), size);

  stbtt_fontinfo font_info;
  stbtt_InitFont(&font_info, font_buffer.data(), 0);

  float scale = stbtt_ScaleForPixelHeight(&font_info, font_sample_point_size);

  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
  int ascent_px = (int)roundf(ascent * scale);
  int descent_px = (int)roundf(descent * scale); // negative value
  int glyph_height = ascent_px - descent_px;

  // Use 'W' (same as your original) as the reference advance
  int adv_raw, lsb;
  stbtt_GetCodepointHMetrics(&font_info, 'W', &adv_raw, &lsb);
  int advance = (int)roundf(adv_raw * scale);

  this->glyph_size = glm::vec2(advance, glyph_height);

  int atlas_w = advance * 10;
  int atlas_h = glyph_height * 10;
  // RGBA atlas, zeroed (transparent black)
  std::vector<uint8_t> atlas(atlas_w * atlas_h * 4, 0);

  // Printable ascii characters (33 - 126)
  for (int cp = 33; cp <= 126; cp++) {
    int col = (cp - 33) % 10;
    int row = (cp - 33) / 10;

    int gw, gh, xoff, yoff;
    uint8_t *bitmap = stbtt_GetCodepointBitmap(&font_info, 0, scale, cp, &gw,
                                               &gh, &xoff, &yoff);
    // yoff is offset from baseline (typically negative = above baseline)

    int base_x = col * advance + xoff;
    int base_y = row * glyph_height + ascent_px + yoff;

    for (int gy = 0; gy < gh; gy++) {
      for (int gx = 0; gx < gw; gx++) {
        int ax = base_x + gx;
        int ay = base_y + gy;
        if (ax < 0 || ax >= atlas_w || ay < 0 || ay >= atlas_h)
          continue;

        int idx = (ay * atlas_w + ax) * 4;
        atlas[idx + 0] = 255;                  // R
        atlas[idx + 1] = 255;                  // G
        atlas[idx + 2] = 255;                  // B
        atlas[idx + 3] = bitmap[gy * gw + gx]; // A
      }
    }

    stbtt_FreeBitmap(bitmap, nullptr);
  }

  font_texture_id = this->load_texture(atlas.data(), atlas_w, atlas_h);

  return;
}

Renderer::~Renderer() {
  for (auto &[path, graphics_pipeline] : graphics_pipelines) {
    SDL_ReleaseGPUGraphicsPipeline(context.device, graphics_pipeline);
  }

  for (auto &[path, texture] : gpu_textures) {
    SDL_ReleaseGPUTexture(context.device, texture);
  }

  for (auto &[name, buffer] : vertex_buffers) {
    SDL_ReleaseGPUBuffer(context.device, buffer);
  }

  for (auto &[name, buffer] : index_buffers) {
    SDL_ReleaseGPUBuffer(context.device, buffer);
  }

  SDL_ReleaseGPUSampler(context.device, clamp_sampler);

  SDL_ReleaseWindowFromGPUDevice(context.device, context.window);
  SDL_DestroyGPUDevice(context.device);
  SDL_DestroyWindow(context.window);

  SDL_Quit();

  return;
}

TextureID Renderer::load_texture(unsigned char *pixels, int w, int h) {
  SDL_GPUTextureCreateInfo texture_info{};
  texture_info.type = SDL_GPU_TEXTURETYPE_2D;
  texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
  texture_info.width = w;
  texture_info.height = h;
  texture_info.layer_count_or_depth = 1;
  texture_info.num_levels = 1;
  // sample_count
  // TODO: GPUTexture should be able to be used as a render target
  // Make use for pixel perfect scaling and post processing

  // TODO: Valgrind error unitialised value create by heap allocation
  SDL_GPUTexture *texture =
      SDL_CreateGPUTexture(this->context.device, &texture_info);
  if (!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create GPU texture: %s", SDL_GetError());
    return -1;
  }

  // Set up transfer buffer
  SDL_GPUTransferBufferCreateInfo texture_transfer_create_info{};
  texture_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  texture_transfer_create_info.size = w * h * 4; // 4 is RGBA8888
  SDL_GPUTransferBuffer *texture_transfer_buffer = SDL_CreateGPUTransferBuffer(
      this->context.device, &texture_transfer_create_info);

  // Transfer data
  void *texture_data_ptr = SDL_MapGPUTransferBuffer(
      this->context.device, texture_transfer_buffer, false);
  SDL_memcpy(texture_data_ptr, (void *)pixels, w * h * 4);

  SDL_UnmapGPUTransferBuffer(this->context.device, texture_transfer_buffer);

  // Start a copy pass
  SDL_GPUCommandBuffer *_command_buffer =
      SDL_AcquireGPUCommandBuffer(this->context.device);
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(_command_buffer);

  // Upload texture data to the GPU texture
  SDL_GPUTextureTransferInfo texture_transfer_info{};
  texture_transfer_info.transfer_buffer = texture_transfer_buffer;
  texture_transfer_info.offset = 0;
  SDL_GPUTextureRegion texture_region{};
  texture_region.texture = texture;
  texture_region.w = w;
  texture_region.h = h;
  texture_region.d = 1; // Depth for 2D texture is 1
  SDL_UploadToGPUTexture(copyPass, &texture_transfer_info, &texture_region,
                         false);

  // End copy pass
  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(_command_buffer);

  SDL_ReleaseGPUTransferBuffer(this->context.device, texture_transfer_buffer);
  // SDL_DestroySurface(image_data);

  gpu_textures[next_texture_id] = texture;

  // SDL_Log("Loaded texture: %s", path.c_str());

  return next_texture_id++;
}

bool Renderer::load_geometry(std::string path, const Vertex *vertices,
                             size_t vertex_size, const Uint16 *indices,
                             size_t index_size) {

  // Create the vertex buffer
  SDL_GPUBufferCreateInfo vertex_buffer_info{};
  vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertex_buffer_info.size = vertex_size;
  SDL_GPUBuffer *vertex_buffer =
      SDL_CreateGPUBuffer(context.device, &vertex_buffer_info);
  if (!vertex_buffer) {
    SDL_Log("Failed to create vertex buffer");
    return false;
  }
  SDL_SetGPUBufferName(context.device, vertex_buffer, "Vertex Buffer");

  // Create the index buffer
  SDL_GPUBufferCreateInfo index_buffer_info{};
  index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  index_buffer_info.size = index_size;
  SDL_GPUBuffer *index_buffer =
      SDL_CreateGPUBuffer(context.device, &index_buffer_info);
  if (!index_buffer) {
    SDL_Log("Failed to create index buffer");
    return false;
  }
  SDL_SetGPUBufferName(context.device, index_buffer, "Index Buffer");

  // Create a transfer buffer to upload to the vertex buffer
  SDL_GPUTransferBufferCreateInfo vertex_transfer_create_info{};
  vertex_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  vertex_transfer_create_info.size = vertex_size + index_size;
  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(context.device, &vertex_transfer_create_info);
  if (!transfer_buffer) {
    SDL_Log("Failed to create transfer buffer");
    return false;
  }

  // Fill the transfer buffer
  Vertex *data = (Vertex *)SDL_MapGPUTransferBuffer(context.device,
                                                    transfer_buffer, false);
  SDL_memcpy(data, (void *)vertices, vertex_size);
  Uint16 *indexData = (Uint16 *)&data[(vertex_size / sizeof(Vertex))];
  SDL_memcpy(indexData, (void *)indices, index_size);
  // Unmap the pointer when you are done updating the transfer buffer
  SDL_UnmapGPUTransferBuffer(context.device, transfer_buffer);

  // Start a copy pass
  SDL_GPUCommandBuffer *_command_buffer =
      SDL_AcquireGPUCommandBuffer(context.device);

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
  SDL_ReleaseGPUTransferBuffer(context.device, transfer_buffer);

  vertex_buffers[path] = vertex_buffer;
  index_buffers[path] = index_buffer;

  return true;
};

bool Renderer::create_graphics_pipeline(std::string path,
                                        SDL_GPUShader *vertex_shader,
                                        SDL_GPUShader *fragment_shader) {
  // Create the graphics pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.vertex_shader = vertex_shader;
  pipeline_info.fragment_shader = fragment_shader;
  // Vertex input state
  static const uint32_t num_vertex_buffers = 1;

  SDL_GPUVertexBufferDescription
      vertex_buffer_descriptions[num_vertex_buffers] = {};
  vertex_buffer_descriptions[0].slot = 0;
  vertex_buffer_descriptions[0].pitch = sizeof(Vertex);
  vertex_buffer_descriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  vertex_buffer_descriptions[0].instance_step_rate = 0;

  pipeline_info.vertex_input_state.vertex_buffer_descriptions =
      vertex_buffer_descriptions;
  pipeline_info.vertex_input_state.num_vertex_buffers = num_vertex_buffers;

  // Vertex attributes
  static const uint32_t vertex_attributes_count = 3;

  SDL_GPUVertexAttribute vertex_attributes[vertex_attributes_count] = {};

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

  pipeline_info.vertex_input_state.num_vertex_attributes =
      vertex_attributes_count;

  pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;

  pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  // Rasterizer state (Unimplemented)
  // Multisample state (Unimplemented)
  // SDL_GPUMultisampleState multisample_state = {};
  // multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_4;
  // multisample_state.sample_mask = 0;
  // multisample_state.enable_mask = false;
  // multisample_state.enable_alpha_to_coverage = false;

  // pipeline_info.multisample_state = multisample_state;

  static const uint32_t num_color_targets = 1;

  SDL_GPUColorTargetDescription color_target_descriptions[num_color_targets];
  color_target_descriptions[0] = {};
  color_target_descriptions[0].format =
      SDL_GetGPUSwapchainTextureFormat(context.device, context.window);
  color_target_descriptions[0].blend_state.src_color_blendfactor =
      SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  color_target_descriptions[0].blend_state.dst_color_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  color_target_descriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
  color_target_descriptions[0].blend_state.src_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  color_target_descriptions[0].blend_state.dst_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  color_target_descriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
  color_target_descriptions[0].blend_state.enable_blend = true;

  pipeline_info.target_info.color_target_descriptions =
      color_target_descriptions;
  pipeline_info.target_info.num_color_targets = num_color_targets;

  SDL_GPUGraphicsPipeline *graphics_pipeline =
      SDL_CreateGPUGraphicsPipeline(context.device, &pipeline_info);

  if (!graphics_pipeline) {
    SDL_Log("Failed to create graphics pipeline");
    return false;
  }

  graphics_pipelines[path] = graphics_pipeline;

  return true;
}

bool Renderer::update_swapchain_texture() {
  _command_buffer = SDL_AcquireGPUCommandBuffer(context.device);

  if (!_command_buffer) {
    SDL_Log("Failed to acquire GPU command buffer");
    return false;
  }

  SDL_WaitAndAcquireGPUSwapchainTexture(_command_buffer, context.window,
                                        &this->swapchain_texture, &this->width,
                                        &this->height);

  return true;
}

bool Renderer::begin_frame() {
  // TODO: Value create by heap allocation valgrind error
  // _command_buffer = SDL_AcquireGPUCommandBuffer(context.device);

  // if (!_command_buffer) {
  //   SDL_Log("Failed to acquire GPU command buffer");
  //   return false;
  // }

  // SDL_GPUTexture *swapchain_texture;
  // SDL_WaitAndAcquireGPUSwapchainTexture(_command_buffer, context.window,
  //                                       &swapchain_texture, &this->width,
  //                                       &this->height);

  SDL_GPUColorTargetInfo color_target_info{};
  color_target_info.texture = this->swapchain_texture;
  color_target_info.clear_color = SDL_FColor{1.0f, 0.0f, 1.0f, 1.0f};
  color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
  color_target_info.store_op = SDL_GPU_STOREOP_STORE;

  _render_pass =
      SDL_BeginGPURenderPass(_command_buffer, &color_target_info, 1, NULL);

  if (!_render_pass) {
    SDL_Log("Failed to begin GPU render pass");
    return false;
  }

  this->projection_matrix =
      glm::ortho(0.0f, (float)this->width / viewport_scale,
                 -(float)this->height / viewport_scale, 0.0f);

  return true;
}

bool Renderer::end_frame() {
  SDL_EndGPURenderPass(_render_pass);

  SDL_SubmitGPUCommandBuffer(_command_buffer);

  this->viewport_scale = SDL_GetWindowPixelDensity(this->context.window);

  return true;
}

// TODO: Add a queue_sprite_load() function to load in unavailable sprites
// TODO: Add a destroy_XX() function to free unused resources
bool Renderer::draw_sprite(TextureID texture_id, glm::vec2 translation,
                           float rotation, glm::vec2 scale, glm::vec4 color) {
  // Bind graphics pipeline
  SDL_BindGPUGraphicsPipeline(_render_pass, graphics_pipelines["SPRITE"]);

  // Bind vertex buffer
  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = vertex_buffers["QUAD"];
  vertex_buffer_bindings[0].offset = 0;

  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);

  // Bind index buffer
  SDL_GPUBufferBinding index_buffer_bindings[1];
  index_buffer_bindings[0].buffer = index_buffers["QUAD"];
  index_buffer_bindings[0].offset = 0;

  SDL_BindGPUIndexBuffer(_render_pass, index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);

  // Uniforms and samplers
  // TODO: conditional jump valgrind error?
  if (gpu_textures.find(texture_id) == gpu_textures.end()) {
    SDL_Log("Sprite not loaded");
    SDL_Quit();
    return false;
  }
  SDL_GPUTextureSamplerBinding fragment_sampler_bindings{};
  fragment_sampler_bindings.texture = gpu_textures[texture_id];
  fragment_sampler_bindings.sampler = clamp_sampler;
  SDL_BindGPUFragmentSamplers(_render_pass,
                              0, // The binding point for the sampler
                              &fragment_sampler_bindings,
                              1 // Number of textures/samplers to bind
  );

  // Calculate uniform values
  sprite_fragment_uniform_buffer.time = SDL_GetTicksNS() / 1e9f;
  sprite_fragment_uniform_buffer.modulate = color;
  SDL_PushGPUFragmentUniformData(_command_buffer, 0,
                                 &sprite_fragment_uniform_buffer,
                                 sizeof(SpriteFragmentUniformBuffer));

  glm::mat4 model_matrix = glm::mat4(1.0f);
  model_matrix = glm::translate(model_matrix,
                                glm::vec3(translation.x, -translation.y, 0.0f));
  model_matrix = glm::rotate(model_matrix, glm::radians(rotation),
                             glm::vec3(0.0f, 0.0f, 1.0f));
  model_matrix = glm::scale(model_matrix, glm::vec3(scale, 1.0f));

  glm::mat4 view_matrix = glm::mat4(1.0f);

  basic_vertex_uniform_buffer.mvp_matrix =
      this->projection_matrix * view_matrix * model_matrix;

  SDL_PushGPUVertexUniformData(_command_buffer, 0, &basic_vertex_uniform_buffer,
                               sizeof(BasicVertexUniformBuffer));

  SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0,
                               0); // TODO: Determine index count

  return true;
}

bool Renderer::draw_color_rect(glm::vec2 position, glm::vec2 size,
                               glm::vec4 color, glm::vec4 corner_radius) {
  // Bind graphics pipeline
  SDL_BindGPUGraphicsPipeline(_render_pass, graphics_pipelines["COLOR_RECT"]);

  // Bind vertex buffer
  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = vertex_buffers["QUAD"];
  vertex_buffer_bindings[0].offset = 0;

  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);

  // Bind index buffer
  SDL_GPUBufferBinding index_buffer_bindings[1];
  index_buffer_bindings[0].buffer = index_buffers["QUAD"];
  index_buffer_bindings[0].offset = 0;

  SDL_BindGPUIndexBuffer(_render_pass, index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);

  // Uniforms and samplers
  // TODO: conditional jump valgrind error?
  // SDL_GPUTextureSamplerBinding fragment_sampler_bindings{};
  // fragment_sampler_bindings.texture = gpu_textures[path];
  // fragment_sampler_bindings.sampler = clamp_sampler;
  // SDL_BindGPUFragmentSamplers(_render_pass,
  //                             0, // The binding point for the sampler
  //                             &fragment_sampler_bindings,
  //                             1 // Number of textures/samplers to bind
  // );

  // Calculate uniform values
  // ui_rect_fragment_uniform_buffer.time = SDL_GetTicksNS() / 1e9f;
  color_rect_fragment_uniform_buffer.modulate = color;
  color_rect_fragment_uniform_buffer.corner_radii = glm::vec4(corner_radius);
  color_rect_fragment_uniform_buffer.size =
      glm::vec4(size.x, size.y, 0.0f, 0.0f);
  SDL_PushGPUFragmentUniformData(_command_buffer, 0,
                                 &color_rect_fragment_uniform_buffer,
                                 sizeof(ColorRectFragmentUniformBuffer));

  glm::mat4 model_matrix = glm::mat4(1.0f);

  model_matrix = glm::translate(model_matrix,
                                glm::vec3(position.x + size.x / 2.0f,
                                          -(position.y + size.y / 2.0f), 0.0f));
  model_matrix = glm::scale(model_matrix, glm::vec3(size, 1.0f));

  basic_vertex_uniform_buffer.mvp_matrix =
      this->projection_matrix * model_matrix;

  SDL_PushGPUVertexUniformData(_command_buffer, 0, &basic_vertex_uniform_buffer,
                               sizeof(BasicVertexUniformBuffer));

  SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0,
                               0); // TODO: Determine index count

  return true;
};

bool Renderer::draw_texture_rect(TextureID texture_id, glm::vec2 position,
                                 glm::vec2 size, glm::vec4 color,
                                 glm::vec4 corner_radius, bool tiling) {
  // Bind graphics pipeline
  SDL_BindGPUGraphicsPipeline(_render_pass, graphics_pipelines["TEXTURE_RECT"]);

  // Bind vertex buffer
  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = vertex_buffers["QUAD"];
  vertex_buffer_bindings[0].offset = 0;

  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);

  // Bind index buffer
  SDL_GPUBufferBinding index_buffer_bindings[1];
  index_buffer_bindings[0].buffer = index_buffers["QUAD"];
  index_buffer_bindings[0].offset = 0;

  SDL_BindGPUIndexBuffer(_render_pass, index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);

  // Uniforms and samplers
  // TODO: conditional jump valgrind error?
  if (gpu_textures.find(texture_id) == gpu_textures.end()) {
    SDL_Log("Sprite not loaded");
    SDL_Quit();
    return false;
  }
  SDL_GPUTextureSamplerBinding fragment_sampler_bindings{};
  fragment_sampler_bindings.texture = gpu_textures[texture_id];
  fragment_sampler_bindings.sampler = tiling ? wrap_sampler : clamp_sampler;
  SDL_BindGPUFragmentSamplers(_render_pass,
                              0, // The binding point for the sampler
                              &fragment_sampler_bindings,
                              1 // Number of textures/samplers to bind
  );

  // Calculate uniform values
  texture_rect_fragment_uniform_buffer.modulate = color;
  texture_rect_fragment_uniform_buffer.corner_radii = glm::vec4(corner_radius);
  texture_rect_fragment_uniform_buffer.size =
      glm::vec4(size.x, size.y, 0.0f, 0.0f);
  texture_rect_fragment_uniform_buffer.tiling = tiling ? 1 : 0;

  SDL_PushGPUFragmentUniformData(_command_buffer, 0,
                                 &texture_rect_fragment_uniform_buffer,
                                 sizeof(TextureRectFragmentUniformBuffer));

  glm::mat4 model_matrix = glm::mat4(1.0f);

  model_matrix = glm::translate(model_matrix,
                                glm::vec3(position.x + size.x / 2.0f,
                                          -(position.y + size.y / 2.0f), 0.0f));
  model_matrix = glm::scale(model_matrix, glm::vec3(size, 1.0f));

  basic_vertex_uniform_buffer.mvp_matrix =
      this->projection_matrix * model_matrix;

  SDL_PushGPUVertexUniformData(_command_buffer, 0, &basic_vertex_uniform_buffer,
                               sizeof(BasicVertexUniformBuffer));

  SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0,
                               0); // TODO: Determine index count

  return true;
}

bool Renderer::draw_text(const char *text, int length, float point_size,
                         glm::vec2 position, glm::vec4 color) {
  // Bind graphics pipeline
  SDL_BindGPUGraphicsPipeline(_render_pass, graphics_pipelines["TEXT"]);

  // Bind vertex buffer
  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = vertex_buffers["QUAD"];
  vertex_buffer_bindings[0].offset = 0;

  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);

  // Bind index buffer
  SDL_GPUBufferBinding index_buffer_bindings[1];
  index_buffer_bindings[0].buffer = index_buffers["QUAD"];
  index_buffer_bindings[0].offset = 0;

  SDL_BindGPUIndexBuffer(_render_pass, index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);

  // Uniforms and samplers
  // TODO: conditional jump valgrind error?
  if (gpu_textures.find(font_texture_id) == gpu_textures.end()) {
    SDL_Log("Sprite not loaded");
    SDL_Quit();
    return false;
  }
  SDL_GPUTextureSamplerBinding fragment_sampler_bindings{};
  fragment_sampler_bindings.texture = gpu_textures[font_texture_id];
  fragment_sampler_bindings.sampler = clamp_sampler;
  SDL_BindGPUFragmentSamplers(_render_pass,
                              0, // The binding point for the sampler
                              &fragment_sampler_bindings,
                              1 // Number of textures/samplers to bind
  );

  float scalar = point_size / font_sample_point_size;

  for (size_t i = 0; i < length; i++) {
    if ((text[i] - 33) == -1) {
      continue;
    }
    // Calculate uniform values
    text_fragment_uniform_buffer.modulate = color;
    float x = static_cast<float>((text[i] - 33) % 10) / 10.0f;
    float y = static_cast<float>(static_cast<int>((text[i] - 33) / 10)) / 10.0f;
    text_fragment_uniform_buffer.uv_rect = glm::vec4(x, y, x + 0.1f, y + 0.1f);
    SDL_PushGPUFragmentUniformData(_command_buffer, 0,
                                   &text_fragment_uniform_buffer,
                                   sizeof(TextFragmentUniformBuffer));

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(
        model_matrix,
        glm::vec3(position.x + (i * glyph_size.x * scalar), -position.y, 0.0f));
    model_matrix =
        glm::scale(model_matrix, glm::vec3(glyph_size * scalar, 1.0f));
    model_matrix = glm::translate(model_matrix, glm::vec3(0.5f, -0.5f, 0.0f));

    glm::mat4 view_matrix = glm::mat4(1.0f);

    text_vertex_uniform_buffer.mvp_matrix =
        this->projection_matrix * view_matrix * model_matrix;
    text_vertex_uniform_buffer.time = SDL_GetTicksNS() / 1e9f;
    text_vertex_uniform_buffer.offset = static_cast<float>(i);

    SDL_PushGPUVertexUniformData(_command_buffer, 0,
                                 &text_vertex_uniform_buffer,
                                 sizeof(TextVertexUniformBuffer));

    SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0,
                                 0); // TODO: Determine index count
  }

  return true;
}

bool Renderer::draw_arc(glm::vec2 position, float radius, float thickness,
                        float rotation, glm::vec4 color) {
  // Bind graphics pipeline
  SDL_BindGPUGraphicsPipeline(_render_pass, graphics_pipelines["ARC"]);

  // Bind vertex buffer
  SDL_GPUBufferBinding vertex_buffer_bindings[1];
  vertex_buffer_bindings[0].buffer = vertex_buffers["QUAD"];
  vertex_buffer_bindings[0].offset = 0;

  SDL_BindGPUVertexBuffers(_render_pass, 0, vertex_buffer_bindings, 1);

  // Bind index buffer
  SDL_GPUBufferBinding index_buffer_bindings[1];
  index_buffer_bindings[0].buffer = index_buffers["QUAD"];
  index_buffer_bindings[0].offset = 0;

  SDL_BindGPUIndexBuffer(_render_pass, index_buffer_bindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);

  // Calculate uniform values
  arc_fragment_uniform_buffer.modulate = color;
  arc_fragment_uniform_buffer.radius = radius;
  arc_fragment_uniform_buffer.thickness = thickness;
  SDL_PushGPUFragmentUniformData(_command_buffer, 0,
                                 &arc_fragment_uniform_buffer,
                                 sizeof(ArcFragmentUniformBuffer));

  glm::mat4 model_matrix = glm::mat4(1.0f);

  model_matrix =
      glm::translate(model_matrix, glm::vec3(position.x, -position.y, 0.0f));
  model_matrix = glm::scale(model_matrix, glm::vec3(radius, radius, 1.0f));
  model_matrix = glm::rotate(model_matrix, glm::radians(rotation),
                             glm::vec3(0.0f, 0.0f, 1.0f));
  model_matrix = glm::translate(model_matrix, glm::vec3(0.5f, 0.5f, 0.0f));

  basic_vertex_uniform_buffer.mvp_matrix =
      this->projection_matrix * model_matrix;

  SDL_PushGPUVertexUniformData(_command_buffer, 0, &basic_vertex_uniform_buffer,
                               sizeof(BasicVertexUniformBuffer));

  SDL_DrawGPUIndexedPrimitives(_render_pass, 6, 1, 0, 0,
                               0); // TODO: Determine index count

  return true;
}

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
