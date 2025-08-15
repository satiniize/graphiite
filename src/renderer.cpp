#include "renderer.hpp"

// Helper
SDL_GPUShader *load_shader(SDL_GPUDevice *device, const char *path,
                           int num_samplers, int num_storage_textures,
                           int num_storage_buffers, int num_uniform_buffers) {
  // Load the shader code
  size_t code_size;
  void *code = SDL_LoadFile(path, &code_size);
  if (code == NULL) {
    SDL_Log("Failed to load shader from disk! %s", path);
    return NULL;
  }
  // Determine shader stage
  SDL_GPUShaderStage stage = SDL_GPU_SHADERSTAGE_VERTEX;
  if (SDL_strstr(path, ".vert")) {
    stage = SDL_GPU_SHADERSTAGE_VERTEX;
  } else if (SDL_strstr(path, ".frag")) {
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

Renderer::Renderer() {}

Renderer::~Renderer() {}

bool Renderer::init() {
  this->context = Context{};
  this->context.title = "Software Renderer";

  // SDL setup
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return false;
  }
  SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

  // Create GPU device
  this->context.device =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  if (this->context.device == NULL) {
    SDL_Log("GPUCreateDevice failed");
    return false;
  }

  // Create window
  SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
  this->context.window = SDL_CreateWindow(this->context.title, WIDTH * scale,
                                          HEIGHT * scale, flags);
  if (!this->context.window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    return false;
  }

  // Claim window and GPU device
  if (!SDL_ClaimWindowForGPUDevice(this->context.device,
                                   this->context.window)) {
    SDL_Log("GPUClaimWindow failed");
    return false;
  }

  // Disable V Sync for FPS testing
  SDL_SetGPUSwapchainParameters(this->context.device, this->context.window,
                                SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                SDL_GPU_PRESENTMODE_IMMEDIATE);

  // Create shaders
  // TODO: make sampler and uniform nums more readable
  SDL_GPUShader *vertex_shader = load_shader(
      this->context.device, "src/shaders/basic.vert.spv", 0, 0, 0, 0);

  SDL_GPUShader *fragment_shader = load_shader(
      this->context.device, "src/shaders/basic.frag.spv", 1, 0, 0, 1);

  // Load test image using SDL_image
  const char *image_path = "res/test.png";

  SDL_Surface *image_data = IMG_Load(image_path);

  if (image_data == NULL) {
    SDL_Log("Could not load image data!");
    return false;
  }

  // Some weird fuckass convention of opengl,
  // Apparently its read backwards so ABGR -> RGBA
  if (image_data->format != SDL_PIXELFORMAT_ABGR8888) {
    SDL_Surface *converted =
        SDL_ConvertSurface(image_data, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(image_data);
    image_data = converted;
  }

  // Create the graphics pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.vertex_shader = vertex_shader;
  pipeline_info.fragment_shader = fragment_shader;
  // Vertex input state
  u_int32_t num_vertex_buffers = 1;

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
  uint32_t vertex_attributes_count = 3;

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
  // Depth stencil state (Unimplemented)
  // Describe the color target
  u_int32_t num_color_targets = 1;

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
  // color_write_mask
  color_target_descriptions[0].blend_state.enable_blend = true;
  // enable_color_write_mask

  pipeline_info.target_info.color_target_descriptions =
      color_target_descriptions;
  pipeline_info.target_info.num_color_targets = num_color_targets;

  graphics_pipeline =
      SDL_CreateGPUGraphicsPipeline(context.device, &pipeline_info);

  if (graphics_pipeline == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create graphics pipeline");
    return false;
  }

  // We don't need to store the shaders after creating the pipeline
  SDL_ReleaseGPUShader(context.device, vertex_shader);
  SDL_ReleaseGPUShader(context.device, fragment_shader);

  // Create gpu sampler
  SDL_GPUSamplerCreateInfo sampler_info{};
  sampler_info.mag_filter =
      SDL_GPU_FILTER_NEAREST; // Smooths pixels when magnified
  sampler_info.min_filter =
      SDL_GPU_FILTER_NEAREST; // Smooths pixels when minified
  sampler_info.mipmap_mode =
      SDL_GPU_SAMPLERMIPMAPMODE_NEAREST; // No mipmaps, so nearest is fine
  sampler_info.address_mode_u =
      SDL_GPU_SAMPLERADDRESSMODE_REPEAT; // Prevents repeating at edges
  sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
  sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

  quad_sampler = SDL_CreateGPUSampler(context.device, &sampler_info);
  if (!quad_sampler) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create GPU sampler: %s", SDL_GetError());
    return false; // Make sure this returns!
  }

  // Create the vertex buffer
  SDL_GPUBufferCreateInfo vertex_buffer_info{};
  vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertex_buffer_info.size = std::size(vertices) * sizeof(Vertex);
  // vertex_buffer_info.props (Unimplemented)

  vertex_buffer = SDL_CreateGPUBuffer(context.device, &vertex_buffer_info);
  if (!vertex_buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create vertex buffer: %s", SDL_GetError());
    SDL_Quit();
    return false;
  }

  SDL_SetGPUBufferName(context.device, vertex_buffer, "Vertex Buffer");

  // Create the index buffer
  SDL_GPUBufferCreateInfo index_buffer_info{};
  index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  index_buffer_info.size = std::size(indices) * sizeof(uint16_t);
  // vertex_buffer_info.props (Unimplemented)

  index_buffer = SDL_CreateGPUBuffer(context.device, &index_buffer_info);
  if (!index_buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create index buffer: %s", SDL_GetError());
    SDL_Quit();
    return false;
  }

  SDL_SetGPUBufferName(context.device, index_buffer, "Index Buffer");

  SDL_GPUTextureCreateInfo texture_info{};
  texture_info.type = SDL_GPU_TEXTURETYPE_2D;
  texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
  texture_info.width = image_data->w;
  texture_info.height = image_data->h;
  texture_info.layer_count_or_depth = 1;
  texture_info.num_levels = 1;
  // sample_count
  // TODO: GPUTexture should be able to be used as a render target
  // Make use for pixel perfect scaling and post processing

  SDL_Log("Creating GPU texture: %dx%d, Format: R8G8B8A8_UNORM",
          texture_info.width, texture_info.height); // ADD THIS LINE
  quad_texture = SDL_CreateGPUTexture(context.device, &texture_info);

  if (!quad_texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create GPU texture: %s", SDL_GetError());
    return false; // Make sure this returns!
  }

  SDL_SetGPUTextureName(context.device, quad_texture, "Amogus Texture");

  // create a transfer buffer to upload to the vertex buffer
  SDL_GPUTransferBufferCreateInfo transferInfo{};
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  transferInfo.size = std::size(vertices) * sizeof(Vertex) +
                      std::size(indices) * sizeof(uint16_t);
  transfer_buffer = SDL_CreateGPUTransferBuffer(context.device, &transferInfo);

  if (!transfer_buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create transfer buffer: %s", SDL_GetError());
    SDL_Quit();
    return false;
  }

  // fill the transfer buffer
  Vertex *data = (Vertex *)SDL_MapGPUTransferBuffer(context.device,
                                                    transfer_buffer, false);
  SDL_memcpy(data, (void *)vertices, std::size(vertices) * sizeof(Vertex));

  Uint16 *indexData = (Uint16 *)&data[std::size(vertices)];
  SDL_memcpy(indexData, (void *)indices, std::size(indices) * sizeof(uint16_t));

  // unmap the pointer when you are done updating the transfer buffer
  SDL_UnmapGPUTransferBuffer(context.device, transfer_buffer);

  // Set up texture data
  SDL_GPUTransferBufferCreateInfo textureTransferInfo{};
  textureTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  textureTransferInfo.size = image_data->w * image_data->h * 4; // 4 is RGBA8888

  SDL_GPUTransferBuffer *textureTransferBuffer =
      SDL_CreateGPUTransferBuffer(context.device, &textureTransferInfo);
  if (!textureTransferBuffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create texture transfer buffer: %s",
                 SDL_GetError());
    return false;
  }

  void *texture_data_ptr =
      SDL_MapGPUTransferBuffer(context.device, textureTransferBuffer, false);

  if (!texture_data_ptr) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to map texture transfer buffer: %s", SDL_GetError());
    // Add cleanup
    SDL_ReleaseGPUTexture(context.device, quad_texture);
    SDL_ReleaseGPUSampler(context.device, quad_sampler);
    SDL_ReleaseGPUTransferBuffer(context.device, textureTransferBuffer);
    return false;
  }

  SDL_memcpy(texture_data_ptr, (void *)image_data->pixels,
             image_data->w * image_data->h * 4);

  SDL_UnmapGPUTransferBuffer(context.device, textureTransferBuffer);

  // Start a copy pass
  SDL_GPUCommandBuffer *commandBuffer =
      SDL_AcquireGPUCommandBuffer(context.device);

  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);

  // where is the data
  SDL_GPUTransferBufferLocation vertex_location{};
  vertex_location.transfer_buffer = transfer_buffer;
  vertex_location.offset = 0;

  // where to upload the data
  SDL_GPUBufferRegion vertex_region{};
  vertex_region.buffer = vertex_buffer;
  vertex_region.size = sizeof(vertices); // TODO: make more robust
  vertex_region.offset = 0;

  // upload the data
  SDL_UploadToGPUBuffer(copyPass, &vertex_location, &vertex_region, false);

  // where is the data
  SDL_GPUTransferBufferLocation index_location{};
  index_location.transfer_buffer = transfer_buffer;
  index_location.offset = std::size(vertices) * sizeof(Vertex);

  // where to upload the data
  SDL_GPUBufferRegion index_region{};
  index_region.buffer = index_buffer;
  index_region.size = std::size(indices) * sizeof(uint16_t);
  index_region.offset = 0;

  SDL_UploadToGPUBuffer(copyPass, &index_location, &index_region, false);

  // Upload texture data to the GPU texture
  SDL_GPUTextureTransferInfo texture_transfer_info{};
  texture_transfer_info.transfer_buffer = textureTransferBuffer;
  texture_transfer_info.offset =
      0; // Start at the beginning of the transfer buffer

  SDL_GPUTextureRegion texture_region{};
  texture_region.texture = quad_texture;
  texture_region.w = image_data->w;
  texture_region.h = image_data->h;
  texture_region.d = 1; // Depth for 2D texture is 1

  SDL_UploadToGPUTexture(copyPass, &texture_transfer_info, &texture_region,
                         false);
  SDL_Log("Called SDL_UploadToGPUTexture.");

  // end the copy pass
  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(commandBuffer);
  SDL_DestroySurface(image_data);
  SDL_ReleaseGPUTransferBuffer(context.device, transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(context.device, textureTransferBuffer);
  // END SDL_GPU

  SDL_Log("Texture uploaded successfully");

  return true;
}

bool Renderer::begin_frame() { return true; }

bool Renderer::end_frame() { return true; }

bool Renderer::loop() {
  // SDL_GPU
  SDL_GPUCommandBuffer *commandBuffer =
      SDL_AcquireGPUCommandBuffer(context.device);

  SDL_GPUTexture *swapchainTexture;
  Uint32 width, height;
  SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, context.window,
                                        &swapchainTexture, &width, &height);

  SDL_GPUColorTargetInfo colorTargetInfo{};
  colorTargetInfo.texture = swapchainTexture;
  colorTargetInfo.clear_color = (SDL_FColor){0.5f, 0.5f, 0.5f, 1.0f};
  colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
  colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

  SDL_GPURenderPass *renderPass =
      SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

  SDL_BindGPUGraphicsPipeline(renderPass, graphics_pipeline);

  // bind the vertex buffer
  SDL_GPUBufferBinding vertexBufferBindings[1];
  vertexBufferBindings[0].buffer = vertex_buffer;
  vertexBufferBindings[0].offset = 0;

  SDL_BindGPUVertexBuffers(renderPass, 0, vertexBufferBindings, 1);

  SDL_GPUBufferBinding indexBufferBindings[1];
  indexBufferBindings[0].buffer = index_buffer;
  indexBufferBindings[0].offset = 0;

  SDL_BindGPUIndexBuffer(renderPass, indexBufferBindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);

  SDL_GPUTextureSamplerBinding fragmentSamplerBinding{};
  fragmentSamplerBinding.texture = quad_texture;
  fragmentSamplerBinding.sampler = quad_sampler;
  SDL_BindGPUFragmentSamplers(renderPass,
                              0, // The binding point/set for the sampler
                                 // (corresponds to layout(binding=0) in shader)
                              &fragmentSamplerBinding,
                              1 // Number of textures/samplers to bind
  );

  timeUniform.time =
      SDL_GetTicksNS() / 1e9f; // the time since the app started in seconds
  SDL_PushGPUFragmentUniformData(commandBuffer, 0, &timeUniform,
                                 sizeof(UniformBuffer));

  SDL_DrawGPUIndexedPrimitives(renderPass, std::size(indices), 1, 0, 0, 0);

  SDL_EndGPURenderPass(renderPass);

  SDL_SubmitGPUCommandBuffer(commandBuffer);

  return true;
}

bool Renderer::cleanup() {
  SDL_ReleaseGPUGraphicsPipeline(context.device, graphics_pipeline);

  SDL_ReleaseGPUBuffer(context.device, vertex_buffer);
  SDL_ReleaseGPUBuffer(context.device, index_buffer);

  SDL_ReleaseGPUTexture(context.device, quad_texture); // <--- ADD THIS
  SDL_ReleaseGPUSampler(context.device, quad_sampler); // <--- ADD THIS

  SDL_ReleaseWindowFromGPUDevice(context.device, context.window);
  SDL_DestroyGPUDevice(context.device);
  SDL_DestroyWindow(context.window);

  SDL_Quit();

  return true;
}
