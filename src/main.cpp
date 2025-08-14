#include <SDL3/SDL.h>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <sys/types.h>
#include <vector>

#define WIDTH 320
#define HEIGHT 180

#include "vec2.hpp"
#include <SDL3_image/SDL_image.h>

#include "config.hpp"

// Entities
#include "component_storage.hpp"
#include "entity_manager.hpp"

// Systems
#include "physics_system.hpp"
#include "rendering_system.hpp"

// Components
#include "rigidbody_component.hpp"
#include "sprite_component.hpp"
#include "transform_component.hpp"

SDL_Window *window;
SDL_GPUDevice *device;
SDL_GPUBuffer *vertex_buffer;
SDL_GPUBuffer *index_buffer;

SDL_GPUTexture *quad_texture;
SDL_GPUSampler *quad_sampler;

// SDL_Surface* for image loading via SDL_image
SDL_Surface *loaded_surface = nullptr;

SDL_GPUTransferBuffer *transfer_buffer;
SDL_GPUGraphicsPipeline *graphics_pipeline;

const int scale = 4;

// Software buffer for screen pixels
uint32_t back_buffer[WIDTH * HEIGHT];

// the vertex input layout
struct Vertex {
  float x, y, z;    // vec3 position
  float r, g, b, a; // vec4 color
  float u, v;       // vec2 texture coordinates
};
// a list of vertices
static Vertex vertices[]{
    {-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}, // top left vertex
    {0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f},  // top right vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
     1.0f}, // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
    // bottom right vertex
};

// the index buffer
static uint16_t indices[]{0, 1, 2, 2, 1, 3};

int init() {
  // SDL setup
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return 1;
  }
  SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

  // Create GPU device
  device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  if (!device) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create GPU device: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  // Scale up for visibility
  window = SDL_CreateWindow("Checkerboard", WIDTH * scale, HEIGHT * scale, 0);
  if (!window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_ClaimWindowForGPUDevice(device, window);
  // SDL_SetGPUSwapchainParameters(device, window,
  //                               SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
  //                               SDL_GPU_PRESENTMODE_IMMEDIATE);

  // load test image using SDL_image
  std::string bitmap_read_name = "./res/ravioli.bmp";
  // loaded_surface = SDL_LoadBMP(bitmap_read_name.c_str());

  loaded_surface =
      SDL_CreateSurface(32, // width (e.g., 32x32 for a small test)
                        32, // height
                        SDL_PIXELFORMAT_RGBA8888 // The desired pixel format
      );
  if (!loaded_surface) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create test surface: %s", SDL_GetError());
    return 1;
  }

  // Fill with a distinct color, e.g., solid red
  // Make sure your texture_info.format later is also
  // SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM
  uint32_t red_pixel =
      0xFF0000FF; // RGBA (Red: FF, Green: 00, Blue: 00, Alpha: FF)

  // Make sure the surface is locked if necessary for direct pixel access
  // (though usually not for RGBA8888) SDL_LockSurface(loaded_surface); //
  // Uncomment if having issues

  for (int y = 0; y < loaded_surface->h; ++y) {
    for (int x = 0; x < loaded_surface->w; ++x) {
      ((uint32_t *)loaded_surface->pixels)[y * loaded_surface->w + x] =
          red_pixel;
    }
  }
  // SDL_UnlockSurface(loaded_surface); // Uncomment if locked

  SDL_Log("Created and filled test surface with red.");
  if (!loaded_surface) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load BMP: %s",
                 SDL_GetError());
    return 1;
  }

  if (loaded_surface->format != SDL_PIXELFORMAT_RGBA8888) {
    SDL_Surface *converted = SDL_ConvertSurface(
        loaded_surface,
        SDL_PIXELFORMAT_RGBA8888); // Use SDL_ConvertSurfaceFormat
    SDL_DestroySurface(loaded_surface);
    loaded_surface = converted;
    if (!loaded_surface) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "Failed to convert surface format to RGBA8888: %s",
                   SDL_GetError());
      return 1;
    }
  }

  // Now it's safe to assume loaded_surface is valid and in RGBA8888
  if (!loaded_surface->pixels) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Loaded surface has NULL pixels after conversion.");
    SDL_DestroySurface(loaded_surface);
    return 1;
  }

  // load the vertex shader code
  size_t vertexCodeSize;
  void *vertexCode = SDL_LoadFile("src/shaders/vertex.spv", &vertexCodeSize);
  if (vertexCode == NULL) {
    SDL_Log("Failed to load shader from disk! %s");
    return NULL;
  }
  // create the vertex shader
  SDL_GPUShaderCreateInfo vertexInfo{};
  vertexInfo.code = (Uint8 *)vertexCode;
  vertexInfo.code_size = vertexCodeSize;
  vertexInfo.entrypoint = "main";
  vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
  vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
  vertexInfo.num_samplers = 0;
  vertexInfo.num_storage_buffers = 0;
  vertexInfo.num_storage_textures = 0;
  vertexInfo.num_uniform_buffers = 0;

  SDL_GPUShader *vertexShader = SDL_CreateGPUShader(device, &vertexInfo);
  if (!vertexShader) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create vertex shader: %s", SDL_GetError());
    SDL_free(vertexCode);
    SDL_Quit();
    return 1;
  } else {
    SDL_Log("Vertex shader created successfully");
  }

  // free the file
  SDL_free(vertexCode);

  // load the fragment shader code
  size_t fragmentCodeSize;
  void *fragmentCode =
      SDL_LoadFile("src/shaders/fragment.spv", &fragmentCodeSize);
  if (fragmentCode == NULL) {
    SDL_Log("Failed to load shader from disk! %s");
    return NULL;
  }
  // create the fragment shader
  SDL_GPUShaderCreateInfo fragmentInfo{};
  fragmentInfo.code = (Uint8 *)fragmentCode;
  fragmentInfo.code_size = fragmentCodeSize;
  fragmentInfo.entrypoint = "main";
  fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
  fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  fragmentInfo.num_samplers = 1;
  fragmentInfo.num_storage_buffers = 0;
  fragmentInfo.num_storage_textures = 0;
  fragmentInfo.num_uniform_buffers = 0;

  SDL_Log("Loading fragment shader");

  SDL_GPUShader *fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);
  if (!fragmentShader) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create fragment shader: %s", SDL_GetError());
    SDL_free(fragmentCode);
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_Quit();
    return 1;
  }
  SDL_Log("Fragment shader created successfully");

  // free the file
  SDL_free(fragmentCode);

  // create the graphics pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.vertex_shader = vertexShader;
  pipelineInfo.fragment_shader = fragmentShader;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

  // describe the vertex buffers
  SDL_GPUVertexBufferDescription vertexBufferDesctiptions[1] = {};
  vertexBufferDesctiptions[0].slot = 0;
  vertexBufferDesctiptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  vertexBufferDesctiptions[0].instance_step_rate = 0;
  vertexBufferDesctiptions[0].pitch = sizeof(Vertex);

  pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions =
      vertexBufferDesctiptions;

  // describe the vertex attribute
  uint32_t vertexAttributesCount = 3;

  SDL_GPUVertexAttribute vertexAttributes[vertexAttributesCount] = {};

  // a_position
  vertexAttributes[0].buffer_slot = 0;
  vertexAttributes[0].location = 0;
  vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  vertexAttributes[0].offset = 0;

  // a_color
  vertexAttributes[1].buffer_slot = 0;
  vertexAttributes[1].location = 1;
  vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
  vertexAttributes[1].offset = sizeof(float) * 3;

  // a_texcoord
  vertexAttributes[2].buffer_slot = 0;
  vertexAttributes[2].location = 2;
  vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
  vertexAttributes[2].offset = sizeof(float) * 7;

  pipelineInfo.vertex_input_state.num_vertex_attributes = vertexAttributesCount;
  pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

  // describe the color target
  SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {};
  colorTargetDescriptions[0] = {};
  // colorTargetDescriptions[0].blend_state.enable_blend = true;
  // colorTargetDescriptions[0].blend_state.color_blend_op =
  // SDL_GPU_BLENDOP_ADD; colorTargetDescriptions[0].blend_state.alpha_blend_op
  // = SDL_GPU_BLENDOP_ADD;
  // colorTargetDescriptions[0].blend_state.src_color_blendfactor =
  //     SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  // colorTargetDescriptions[0].blend_state.dst_color_blendfactor =
  //     SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  // colorTargetDescriptions[0].blend_state.src_alpha_blendfactor =
  //     SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  // colorTargetDescriptions[0].blend_state.dst_alpha_blendfactor =
  //     SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  colorTargetDescriptions[0].format =
      SDL_GetGPUSwapchainTextureFormat(device, window);

  pipelineInfo.target_info.num_color_targets = 1;
  pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

  // Debug: Check device and shaders before creating the pipeline
  if (!device) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Device is NULL before pipeline creation!");
  }
  if (!vertexShader) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Vertex shader is NULL before pipeline creation!");
  }
  if (!fragmentShader) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Fragment shader is NULL before pipeline creation!");
  }
  if (!pipelineInfo.vertex_input_state.vertex_buffer_descriptions) {
    SDL_LogError(
        SDL_LOG_CATEGORY_APPLICATION,
        "Vertex buffer descriptions is NULL before pipeline creation!");
  }
  if (!pipelineInfo.vertex_input_state.vertex_attributes) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Vertex attributes is NULL before pipeline creation!");
  }
  if (!pipelineInfo.target_info.color_target_descriptions) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Color target descriptions is NULL before pipeline creation!");
  }
  SDL_Log("here");
  graphics_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
  SDL_Log("here2");

  if (graphics_pipeline == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create graphics pipeline");
    return -1;
  }

  // we don't need to store the shaders after creating the pipeline
  SDL_ReleaseGPUShader(device, vertexShader);
  SDL_ReleaseGPUShader(device, fragmentShader);

  // create the vertex buffer
  SDL_GPUBufferCreateInfo vertex_bufferInfo{};
  vertex_bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertex_bufferInfo.size = sizeof(vertices);
  // TODO
  // vertex_bufferInfo.props =
  vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_bufferInfo);
  if (!vertex_buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create vertex buffer: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_SetGPUBufferName(device, vertex_buffer, "Vertex Buffer");

  // create the index buffer
  SDL_GPUBufferCreateInfo index_bufferInfo{};
  index_bufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  index_bufferInfo.size = sizeof(indices);
  // TODO
  // vertex_bufferInfo.props =
  index_buffer = SDL_CreateGPUBuffer(device, &index_bufferInfo);
  if (!index_buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create index buffer: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_SetGPUBufferName(device, index_buffer, "Index Buffer");

  // Texture = SDL_CreateGPUTexture(
  //     context->Device, &(SDL_GPUTextureCreateInfo){
  //                          .type = SDL_GPU_TEXTURETYPE_2D,
  //                          .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
  //                          .width = imageData->w,
  //                          .height = imageData->h,
  //                          .layer_count_or_depth = 1,
  //                          .num_levels = 1,
  //                          .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER});
  // SDL_SetGPUTextureName(context->Device, Texture, "Ravioli Texture");

  SDL_GPUTextureCreateInfo texture_info{};
  texture_info.type = SDL_GPU_TEXTURETYPE_2D;
  texture_info.format =
      SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM; // Assuming your Bitmap outputs RGBA
                                            // 8888
  texture_info.width = loaded_surface->w;
  texture_info.height = loaded_surface->h;
  texture_info.layer_count_or_depth = 1;
  texture_info.num_levels =
      1; // Mipmapping would require more levels, but 1 is fine for now
  texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER; // Needs to be sampled
  // destination

  SDL_Log("Creating GPU texture: %dx%d, Format: R8G8B8A8_UNORM",
          texture_info.width, texture_info.height); // ADD THIS LINE
  quad_texture = SDL_CreateGPUTexture(device, &texture_info);
  if (!quad_texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create GPU texture: %s", SDL_GetError());
    return -1; // Make sure this returns!
  } else {
    SDL_Log("Successfully created GPU texture.");
  }
  SDL_SetGPUTextureName(device, quad_texture, "Amogus Texture");

  // create gpu sampler
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
  // Add other sampler settings if needed (anisotropy, LOD bias, etc.)

  quad_sampler = SDL_CreateGPUSampler(device, &sampler_info);
  if (!quad_sampler) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create GPU sampler: %s", SDL_GetError());
    SDL_ReleaseGPUTexture(device, quad_texture);
    return -1; // Make sure this returns!
  } else {
    SDL_Log("Successfully created GPU sampler.");
  } // SDL_SetGPUSamplerName(device, quad_sampler, "Amogus Sampler");

  // create a transfer buffer to upload to the vertex buffer
  SDL_GPUTransferBufferCreateInfo transferInfo{};
  transferInfo.size = sizeof(vertices) + sizeof(indices); // Add indices size
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
  if (!transfer_buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create transfer buffer: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  // fill the transfer buffer
  Vertex *data =
      (Vertex *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
  SDL_memcpy(data, (void *)vertices, sizeof(vertices));

  Uint16 *indexData = (Uint16 *)&data[std::size(vertices)];
  SDL_memcpy(indexData, (void *)indices, sizeof(indices));

  // unmap the pointer when you are done updating the transfer buffer
  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

  // Set up texture data
  SDL_GPUTransferBufferCreateInfo textureTransferInfo{};
  textureTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  // textureTransferInfo.size = bmp_read.get_width() * bmp_read.get_height();
  textureTransferInfo.size = loaded_surface->w * loaded_surface->h * 4;
  SDL_Log("Texture transfer buffer size: %u bytes",
          textureTransferInfo.size); // ADD THIS LINE
  SDL_GPUTransferBuffer *textureTransferBuffer =
      SDL_CreateGPUTransferBuffer(device, &textureTransferInfo);
  if (!textureTransferBuffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create texture transfer buffer: %s",
                 SDL_GetError());
    // Add cleanup for texture/sampler here
    SDL_ReleaseGPUTexture(device, quad_texture);
    SDL_ReleaseGPUSampler(device, quad_sampler);
    return -1; // Make sure this returns!
  } else {
    SDL_Log("Successfully created texture transfer buffer.");
  }
  // 4 bytes per pixel (RGBA)

  if (!textureTransferBuffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to create texture transfer buffer: %s",
                 SDL_GetError());
    // ... release quad_texture, quad_sampler ...
    return -1;
  }

  void *texture_data_ptr =
      SDL_MapGPUTransferBuffer(device, textureTransferBuffer, false);
  if (!texture_data_ptr) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to map texture transfer buffer: %s", SDL_GetError());
    // Add cleanup
    SDL_ReleaseGPUTexture(device, quad_texture);
    SDL_ReleaseGPUSampler(device, quad_sampler);
    SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);
    return -1;
  } else {
    SDL_Log("Successfully mapped texture transfer buffer.");
  }

  SDL_memcpy(texture_data_ptr, loaded_surface->pixels,
             loaded_surface->w * loaded_surface->h * 4);
  SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

  // start a copy pass
  SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
  if (!commandBuffer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to acquire command buffer for upload: %s",
                 SDL_GetError());
    // Add cleanup
    SDL_ReleaseGPUTexture(device, quad_texture);
    SDL_ReleaseGPUSampler(device, quad_sampler);
    SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);
    return -1;
  } else {
    SDL_Log("Successfully acquired command buffer for upload.");
  }
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);
  if (!copyPass) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to begin copy pass: %s",
                 SDL_GetError());
    SDL_SubmitGPUCommandBuffer(
        commandBuffer); // IMPORTANT: release if beginning fails
    // Add cleanup
    SDL_ReleaseGPUTexture(device, quad_texture);
    SDL_ReleaseGPUSampler(device, quad_sampler);
    SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);
    return -1;
  } else {
    SDL_Log("Successfully began copy pass.");
  }
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
  index_location.offset = sizeof(vertices);

  // where to upload the data
  SDL_GPUBufferRegion index_region{};
  index_region.buffer = index_buffer;
  index_region.size = sizeof(indices);
  index_region.offset = 0;

  SDL_UploadToGPUBuffer(copyPass, &index_location, &index_region, false);

  // Upload texture data to the GPU texture
  SDL_GPUTextureTransferInfo texture_transfer_info{};
  texture_transfer_info.transfer_buffer = textureTransferBuffer;
  texture_transfer_info.offset =
      0; // Start at the beginning of the transfer buffer

  SDL_GPUTextureRegion texture_region{};
  texture_region.texture = quad_texture;
  texture_region.w = loaded_surface->w;
  texture_region.h = loaded_surface->h;
  texture_region.d = 1; // Depth for 2D texture is 1
  // texture_region.x = 0;
  // texture_region.y = 0;
  // texture_region.z = 0; // Destination offset

  SDL_UploadToGPUTexture(copyPass, &texture_transfer_info, &texture_region,
                         false);
  SDL_Log("Called SDL_UploadToGPUTexture.");
  // // Upload texture
  // SDL_UploadToGPUTexture(
  //     copyPass,
  //     &(SDL_GPUTextureTransferInfo){
  //         .transfer_buffer = textureTransferBuffer,
  //         .offset = 0, /* Zeros out the rest */
  //     },
  //     &(SDL_GPUTextureRegion){
  //         .texture = Texture, .w = imageData->w, .h = imageData->h, .d =
  //         1},
  //     false);

  // end the copy pass
  SDL_EndGPUCopyPass(copyPass);
  SDL_Log("Successfully ended copy pass.");
  SDL_SubmitGPUCommandBuffer(commandBuffer);
  SDL_Log("Successfully submitted command buffer.");
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);
  SDL_DestroySurface(loaded_surface);
  SDL_Log("Released transfer buffers and surface.");
  // END SDL_GPU

  SDL_Log("Texture uploaded successfully");

  return 0;
}

void loop() {
  // SDL_GPU
  SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);

  if (commandBuffer == NULL) {
    return;
  }

  SDL_GPUTexture *swapchainTexture;
  Uint32 width, height;
  SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window,
                                        &swapchainTexture, &width, &height);

  if (swapchainTexture == NULL) {
    // you must always submit the command buffer
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return;
  }

  SDL_GPUColorTargetInfo colorTargetInfo{};
  colorTargetInfo.texture = swapchainTexture;
  colorTargetInfo.clear_color = (SDL_FColor){0.0f, 1.0f, 0.0f, 1.0f};
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

  SDL_DrawGPUIndexedPrimitives(renderPass, std::size(indices), 1, 0, 0, 0);

  SDL_EndGPURenderPass(renderPass);

  SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void cleanup() {
  SDL_ReleaseGPUBuffer(device, vertex_buffer);
  SDL_ReleaseGPUBuffer(device, index_buffer);

  SDL_ReleaseGPUTexture(device, quad_texture); // <--- ADD THIS
  SDL_ReleaseGPUSampler(device, quad_sampler); // <--- ADD THIS

  SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline);

  SDL_DestroyGPUDevice(device);

  SDL_DestroyWindow(window);
  SDL_Quit();
}

std::string strip_lr(const std::string &str) {
  int start = 0;
  while (start < str.length() &&
         std::isspace(static_cast<unsigned char>(str[start]))) {
    ++start;
  }
  int end = str.length() - 1;
  while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
    end--;
  }
  return str.substr(start, end - start + 1);
}

int main(int argc, char *argv[]) {
  // std::string toml_file_name = "./res/test.toml";
  // std::string bitmap_write_name = "./res/test_image_write.bmp";
  // std::string bitmap_read_name = "./res/test_image_read.bmp";

  // std::ifstream toml_file(toml_file_name);
  // if (!toml_file) {
  //   SDL_Log("Could not open toml");
  //   return 1;
  // }
  // std::string line;

  // while (std::getline(toml_file, line)) {
  //   // Handle comments
  //   int comment_pos = line.find("#");
  //   if (comment_pos != std::string::npos) {
  //     line = line.substr(0, comment_pos);
  //   }
  //   // Handle tables
  //   // Handle key-value pairs
  //   int equal_sign_pos = line.find("=");
  //   if (equal_sign_pos == std::string::npos) {
  //     continue;
  //   }
  //   std::string key = strip_lr(line.substr(0, equal_sign_pos));
  //   std::string raw_value =
  //       strip_lr(line.substr(equal_sign_pos + 1, line.length()));
  //   // Boolean true
  //   if (raw_value == "true") {
  //   }
  //   // Boolean false
  //   if (raw_value == "false") {
  //   }
  //   // Float
  //   if (raw_value.find(".") != std::string::npos) {
  //   }
  //   // vec2
  //   if (raw_value.starts_with("vec2")) {
  //   }
  //   std::cout << key << ":" << raw_value << std::endl;
  // }

  // int bitmap_write_width = 16;
  // int bitmap_write_height = 16;

  // Create a test image (gradient)
  // Bitmap bmp_write(bitmap_write_width, bitmap_write_height);

  // for (int y = 0; y < bitmap_write_height; ++y) {
  //   for (int x = 0; x < bitmap_write_width; ++x) {
  //     uint8_t r =
  //         static_cast<int>(static_cast<float>(x) / bitmap_write_width *
  //         255.0f);
  //     uint8_t g = static_cast<int>(static_cast<float>(y) /
  //     bitmap_write_height *
  //                                  255.0f);
  //     uint8_t b = 0;
  //     bmp_write.set_pixel(x, y, (r << 24) | (g << 16) | (b << 8) | 0xFF);
  //   }
  // }

  // Dump BMP file
  // if (!bmp_write.dump(bitmap_write_name)) {
  //   std::cerr << "Failed to write BMP file.\n";
  //   return 1;
  // }

  // Read BMP file using constructor
  // Bitmap bmp_read(bitmap_read_name);

  // int bitmap_read_width = bmp_read.get_width();
  // int bitmap_read_height = bmp_read.get_height();

  // std::cout << "Width: " << bitmap_read_width << std::endl;
  // std::cout << "Height: " << bitmap_read_height << std::endl;

  int success = init();

  if (success != 0) {
    return success;
  }

  // EntityManager entity_manager;

  // Cursor
  // EntityID cursor = entity_manager.create();
  // SpriteComponent cursor_sprite = {
  //     .bitmap = bmp_write,
  //     .size = vec2(bmp_write.get_width(), bmp_write.get_height())};
  // sprite_components[cursor] = cursor_sprite;
  // TransformComponent cursor_transform;
  // transform_components[cursor] = cursor_transform;

  // Player Amogus
  // EntityID amogus = entity_manager.create();
  // SpriteComponent sprite_component = {
  //     .bitmap = bmp_read,
  //     .size = vec2(bitmap_read_width, bitmap_read_height),
  // };
  // sprite_components[amogus] = sprite_component;
  // TransformComponent transform_component = {
  //     .position = vec2(64.0f, 64.0f),
  // };
  // transform_components[amogus] = transform_component;
  // RigidBodyComponent rigidbody_component = {
  //     .collision_aabb = AABB(vec2(-8.0f, -8.0f), vec2(8.0f, 8.0f)),
  // };
  // rigidbody_components[amogus] = rigidbody_component;

  // std::mt19937 random_engine;
  // std::uniform_real_distribution<float> velocity_distribution(-1.0f, 1.0f);
  // std::uniform_real_distribution<float> position_distribution(0.0f, 1.0f);
  // float velocity_magnitude = 512.0f;
  // int friends = 16;
  // for (int i = 0; i < friends; ++i) {
  //   EntityID amogus2 = entity_manager.create();
  //   SpriteComponent sprite_component2 = {
  //       .bitmap = bmp_read,
  //       .size = vec2(bitmap_read_width, bitmap_read_height)};
  //   sprite_components[amogus2] = sprite_component2;
  //   TransformComponent transform_component2;
  //   transform_component2.position =
  //       vec2(16.0f + (static_cast<float>(WIDTH) - 32.0f) *
  //                        position_distribution(random_engine),
  //            16.0f + (static_cast<float>(HEIGHT) - 32.0f) *
  //                        position_distribution(random_engine));
  //   transform_components[amogus2] = transform_component2;
  //   RigidBodyComponent rigidbody_component2;
  //   rigidbody_component2.collision_aabb =
  //       AABB(vec2(-8.0f, -8.0f), vec2(8.0f, 8.0f));
  //   rigidbody_component2.velocity =
  //       vec2(velocity_magnitude * velocity_distribution(random_engine),
  //            velocity_magnitude * velocity_distribution(random_engine));
  //   rigidbody_components[amogus2] = rigidbody_component2;
  // }
  // EntityID top_wall = entity_manager.create();
  // StaticBodyComponent top_wall_staticbody_component;
  // TransformComponent top_wall_transform_component;
  // top_wall_transform_component.position = vec2(WIDTH / 2.0f, -8.0f);
  // top_wall_staticbody_component.collision_aabb =
  //     AABB(vec2(-WIDTH / 2.0f, -8.0f), vec2(WIDTH / 2.0f, 8.0f));
  // staticbody_components[top_wall] = top_wall_staticbody_component;
  // transform_components[top_wall] = top_wall_transform_component;

  // EntityID bottom_wall = entity_manager.create();
  // StaticBodyComponent bottom_wall_staticbody_component;
  // TransformComponent bottom_wall_transform_component;
  // bottom_wall_transform_component.position = vec2(WIDTH / 2.0f, HEIGHT
  // + 8.0f); bottom_wall_staticbody_component.collision_aabb =
  //     AABB(vec2(-WIDTH / 2.0f, -8.0f), vec2(WIDTH / 2.0f, 8.0f));
  // staticbody_components[bottom_wall] = bottom_wall_staticbody_component;
  // transform_components[bottom_wall] = bottom_wall_transform_component;

  // EntityID left_wall = entity_manager.create();
  // StaticBodyComponent left_wall_staticbody_component;
  // TransformComponent left_wall_transform_component;
  // left_wall_transform_component.position = vec2(-8.0f, HEIGHT / 2.0f);
  // left_wall_staticbody_component.collision_aabb =
  //     AABB(vec2(-8.0f, -HEIGHT / 2.0f), vec2(8.0f, HEIGHT / 2.0f));
  // staticbody_components[left_wall] = left_wall_staticbody_component;
  // transform_components[left_wall] = left_wall_transform_component;

  // EntityID right_wall = entity_manager.create();
  // StaticBodyComponent right_wall_staticbody_component;
  // TransformComponent right_wall_transform_component;
  // right_wall_transform_component.position = vec2(WIDTH + 8.0f, HEIGHT
  // / 2.0f); right_wall_staticbody_component.collision_aabb =
  //     AABB(vec2(-8.0f, -HEIGHT / 2.0f), vec2(8.0f, HEIGHT / 2.0f));
  // staticbody_components[right_wall] = right_wall_staticbody_component;
  // transform_components[right_wall] = right_wall_transform_component;

  // float physics_frame_rate = 60.0f;

  uint32_t prev_frame_tick = SDL_GetTicks();
  float physics_delta_time = 1.0f / physics_tick_rate;
  float process_delta_time = 0.0f;
  float accumulator = 0.0;

  int physics_frame_count = 0;
  int process_frame_count = 0;
  float time_scale = 1.0f;

  bool running = true;

  while (running) {
    uint32_t frame_tick = SDL_GetTicks();
    process_delta_time =
        static_cast<float>(frame_tick - prev_frame_tick) / 1000.0f;
    accumulator += process_delta_time;
    prev_frame_tick = frame_tick;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    const bool *keystate = SDL_GetKeyboardState(NULL);

    ++process_frame_count;
    if (accumulator >= (physics_delta_time / time_scale)) {
      accumulator -= (physics_delta_time / time_scale);
      ++physics_frame_count;
      if (physics_frame_count >= static_cast<int>(physics_tick_rate)) {
        SDL_Log("FPS: %d", static_cast<int>(process_frame_count));
        physics_frame_count = 0;
        process_frame_count = 0;
      }
      // Get entity transform and rigidbody
      // TransformComponent &amogus_transform = transform_components[amogus];
      // RigidBodyComponent &amogus_rigidbody = rigidbody_components[amogus];

      // Player specific code
      // WASD Movement
      // float accel = 512.0f;
      // vec2 move_dir = vec2(0.0f, 0.0f);
      // if (keystate[SDL_SCANCODE_W]) {
      //   move_dir.y -= 1.0f;
      // }
      // if (keystate[SDL_SCANCODE_A]) {
      //   move_dir.x -= 1.0f;
      // }
      // if (keystate[SDL_SCANCODE_S]) {
      //   move_dir.y += 1.0f;
      // }
      // if (keystate[SDL_SCANCODE_D]) {
      //   move_dir.x += 1.0f;
      // }
      // amogus_rigidbody.velocity += move_dir * accel * physics_delta_time;

      // Showcase rotation
      // amogus_transform.rotation += 2.0f * physics_delta_time;

      // Physics tick-rate ECS system
      // PhysicsSystem::Update(physics_delta_time);
    }
    // float cursor_x;
    // float cursor_y;

    // SDL_GetMouseState(&cursor_x, &cursor_y);

    // TransformComponent &cursorTransform = transform_components[cursor];
    // cursorTransform.position = vec2(cursor_x, cursor_y) / scale;

    // --- Software rendering: fill the pixel buffer (checkerboard) ---
    // int checker_size = 1;
    // for (int y = 0; y < HEIGHT; ++y) {
    //   for (int x = 0; x < WIDTH; ++x) {
    //     int checker = ((x / checker_size) + (y / checker_size)) % 2;
    //     uint8_t v = checker ? 192 : 128;
    //     back_buffer[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
    //   }
    // }

    // Process tick-rate ECS sytems
    // SpriteSystem::update_aabbs();
    // SpriteSystem::draw_all(back_buffer, WIDTH, HEIGHT);

    loop();
  }
  cleanup();
  return 0;
}
