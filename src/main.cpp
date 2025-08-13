#include <SDL3/SDL.h>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#define WIDTH 320
#define HEIGHT 180

#include "bitmap.hpp"
#include "vec2.hpp"

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
SDL_GPUTransferBuffer *transfer_buffer;
SDL_GPUGraphicsPipeline *graphics_pipeline;
// SDL_Renderer *renderer;
// SDL_Texture *texture;

const int scale = 4;

// Software buffer for screen pixels
uint32_t back_buffer[WIDTH * HEIGHT];

// the vertex input layout
struct Vertex {
  float x, y, z;    // vec3 position
  float r, g, b, a; // vec4 color
};

// a list of vertices
static Vertex vertices[]{
    {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},   // top vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}   // bottom right vertex
};

// the index buffer
static uint16_t indices[]{0, 1, 2};

int init() {
  // SDL setup
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return 1;
  }
  SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

  // Scale up for visibility
  window = SDL_CreateWindow("Checkerboard", WIDTH * scale, HEIGHT * scale, 0);
  if (!window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  // Create GPU device
  device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  SDL_ClaimWindowForGPUDevice(device, window);
  SDL_SetGPUSwapchainParameters(device, window,
                                SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                SDL_GPU_PRESENTMODE_IMMEDIATE);

  // load the vertex shader code
  size_t vertexCodeSize;
  void *vertexCode = SDL_LoadFile("src/shaders/vertex.spv", &vertexCodeSize);

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

  // free the file
  SDL_free(vertexCode);

  // load the fragment shader code
  size_t fragmentCodeSize;
  void *fragmentCode =
      SDL_LoadFile("src/shaders/fragment.spv", &fragmentCodeSize);

  // create the fragment shader
  SDL_GPUShaderCreateInfo fragmentInfo{};
  fragmentInfo.code = (Uint8 *)fragmentCode;
  fragmentInfo.code_size = fragmentCodeSize;
  fragmentInfo.entrypoint = "main";
  fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
  fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  fragmentInfo.num_samplers = 0;
  fragmentInfo.num_storage_buffers = 0;
  fragmentInfo.num_storage_textures = 0;
  fragmentInfo.num_uniform_buffers = 0;

  SDL_GPUShader *fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);

  // free the file
  SDL_free(fragmentCode);

  // create the graphics pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.vertex_shader = vertexShader;
  pipelineInfo.fragment_shader = fragmentShader;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

  // describe the vertex buffers
  SDL_GPUVertexBufferDescription vertexBufferDesctiptions[1];
  vertexBufferDesctiptions[0].slot = 0;
  vertexBufferDesctiptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  vertexBufferDesctiptions[0].instance_step_rate = 0;
  vertexBufferDesctiptions[0].pitch = sizeof(Vertex);

  pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions =
      vertexBufferDesctiptions;

  // describe the vertex attribute
  SDL_GPUVertexAttribute vertexAttributes[2];

  // a_position
  vertexAttributes[0].buffer_slot = 0;
  vertexAttributes[0].location = 0;
  vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  vertexAttributes[0].offset = 0;

  // a_color
  vertexAttributes[1].buffer_slot = 0;
  vertexAttributes[1].location = 1;
  vertexAttributes[1].format =
      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; // TODO, add UV data
  vertexAttributes[1].offset = sizeof(float) * 3;

  pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
  pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

  // describe the color target
  SDL_GPUColorTargetDescription colorTargetDescriptions[1];
  colorTargetDescriptions[0] = {};
  colorTargetDescriptions[0].blend_state.enable_blend = true;
  colorTargetDescriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
  colorTargetDescriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
  colorTargetDescriptions[0].blend_state.src_color_blendfactor =
      SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  colorTargetDescriptions[0].blend_state.dst_color_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  colorTargetDescriptions[0].blend_state.src_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  colorTargetDescriptions[0].blend_state.dst_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  colorTargetDescriptions[0].format =
      SDL_GetGPUSwapchainTextureFormat(device, window);

  pipelineInfo.target_info.num_color_targets = 1;
  pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

  // create the pipeline
  graphics_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

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
  vertex_bufferInfo.size = sizeof(vertices);
  vertex_bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_bufferInfo);

  SDL_SetGPUBufferName(device, vertex_buffer, "Vertex Buffer");

  // Uncomment when vertices modified
  // create the index buffer
  SDL_GPUBufferCreateInfo index_bufferInfo{};
  index_bufferInfo.size = sizeof(indices);
  index_bufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  index_buffer = SDL_CreateGPUBuffer(device, &index_bufferInfo);

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
  // SDL_SetGPUTextureName(context->Device, Texture, "Ravioli Texture 🖼️");

  // create a transfer buffer to upload to the vertex buffer
  SDL_GPUTransferBufferCreateInfo transferInfo{};
  transferInfo.size = sizeof(vertices) + sizeof(indices); // Add indices size
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

  // fill the transfer buffer
  Vertex *data =
      (Vertex *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

  // SDL_memcpy(data, (void *)vertices, sizeof(vertices));
  data[0] = vertices[0];
  data[1] = vertices[1];
  data[2] = vertices[2];

  Uint16 *indexData = (Uint16 *)&data[3];
  indexData[0] = indices[0];
  indexData[1] = indices[1];
  indexData[2] = indices[2];

  // unmap the pointer when you are done updating the transfer buffer
  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

  // // Set up texture data
  // SDL_GPUTransferBuffer *textureTransferBuffer = SDL_CreateGPUTransferBuffer(
  //     context->Device, &(SDL_GPUTransferBufferCreateInfo){
  //                          .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
  //                          .size = imageData->w * imageData->h * 4}); // RGBA
  //                          8888

  // Uint8 *textureTransferPtr =
  //     SDL_MapGPUTransferBuffer(context->Device, textureTransferBuffer,
  //     false);
  // SDL_memcpy(textureTransferPtr, imageData->pixels,
  //            imageData->w * imageData->h * 4);
  // SDL_UnmapGPUTransferBuffer(context->Device, textureTransferBuffer);

  // start a copy pass
  SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);

  // where is the data
  SDL_GPUTransferBufferLocation vertex_location{};
  vertex_location.transfer_buffer = transfer_buffer;
  vertex_location.offset = 0;

  // where to upload the data
  SDL_GPUBufferRegion vertex_region{};
  vertex_region.buffer = vertex_buffer;
  vertex_region.size = sizeof(vertices);
  vertex_region.offset = 0;

  // upload the data
  // SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
  SDL_UploadToGPUBuffer(copyPass, &vertex_location, &vertex_region, false);

  // Upload vertex data
  // SDL_UploadToGPUBuffer(
  //     copyPass,
  //     &(SDL_GPUTransferBufferLocation){.transfer_buffer =
  //     bufferTransferBuffer,
  //                                      .offset = 0},
  //     &(SDL_GPUBufferRegion){.buffer = VertexBuffer,
  //                            .offset = 0,
  //                            .size = sizeof(PositionTextureVertex) * 4},
  //     false);

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

  // Upload index data
  // SDL_UploadToGPUBuffer(copyPass,
  //                       &(SDL_GPUTransferBufferLocation){
  //                           .transfer_buffer = bufferTransferBuffer,
  //                           .offset = sizeof(PositionTextureVertex) * 4},
  //                       &(SDL_GPUBufferRegion){.buffer = IndexBuffer,
  //                                              .offset = 0,
  //                                              .size = sizeof(Uint16) * 6},
  //                       false);

  // Upload texture

  // SDL_UploadToGPUTexture(
  //     copyPass,
  //     &(SDL_GPUTextureTransferInfo){
  //         .transfer_buffer = textureTransferBuffer,
  //         .offset = 0, /* Zeros out the rest */
  //     },
  //     &(SDL_GPUTextureRegion){
  //         .texture = Texture, .w = imageData->w, .h = imageData->h, .d = 1},
  //     false);

  // end the copy pass
  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(commandBuffer);
  // END SDL_GPU

  // renderer = SDL_CreateRenderer(window, NULL);
  // if (!renderer) {
  //   SDL_Log("Couldn't create renderer: %s", SDL_GetError());
  //   SDL_DestroyWindow(window);
  //   SDL_Quit();
  //   return 1;
  // }

  // texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
  //                             SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  // SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  // if (!texture) {
  //   SDL_Log("Couldn't create texture: %s", SDL_GetError());
  //   SDL_DestroyRenderer(renderer);
  //   SDL_DestroyWindow(window);
  //   SDL_Quit();
  //   return 1;
  // }
  return 0;
}

void loop() {
  // Upload pixel buffer to texture
  // void *front_buffer;
  // int pitch;
  // SDL_LockTexture(texture, NULL, &front_buffer, &pitch);
  // memcpy(front_buffer, back_buffer, WIDTH * HEIGHT * sizeof(uint32_t));
  // SDL_UnlockTexture(texture);

  // // --- Render the texture to the window ---
  // SDL_RenderClear(renderer);
  // SDL_FRect dst = {0, 0, WIDTH * scale, HEIGHT * scale};
  // SDL_RenderTexture(renderer, texture, NULL, &dst);
  // SDL_RenderPresent(renderer);

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
  colorTargetInfo.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
  colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
  colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

  SDL_GPURenderPass *renderPass =
      SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

  SDL_BindGPUGraphicsPipeline(renderPass, graphics_pipeline);

  // bind the vertex buffer
  SDL_GPUBufferBinding vertexBufferBindings[1];
  vertexBufferBindings[0].buffer =
      vertex_buffer;                  // index 0 is slot 0 in this example
  vertexBufferBindings[0].offset = 0; // start from the first byte

  SDL_BindGPUVertexBuffers(renderPass, 0, vertexBufferBindings,
                           1); // bind one buffer starting from slot 0

  SDL_GPUBufferBinding indexBufferBindings[1];
  indexBufferBindings[0].buffer =
      index_buffer;                  // index 0 is slot 0 in this example
  indexBufferBindings[0].offset = 0; // start from the first byte

  SDL_BindGPUIndexBuffer(renderPass, indexBufferBindings,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);

  // SDL_BindGPUGraphicsPipeline(renderPass, Pipeline);
  // SDL_BindGPUVertexBuffers(
  //     renderPass, 0,
  //     &(SDL_GPUBufferBinding){.buffer = VertexBuffer, .offset = 0}, 1);
  // SDL_BindGPUIndexBuffer(
  //     renderPass, &(SDL_GPUBufferBinding){.buffer = IndexBuffer, .offset =
  //     0}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
  // SDL_BindGPUFragmentSamplers(
  //     renderPass, 0,
  //     &(SDL_GPUTextureSamplerBinding){.texture = Texture,
  //                                     .sampler =
  //                                     Samplers[CurrentSamplerIndex]},
  //     1);
  SDL_DrawGPUIndexedPrimitives(renderPass, std::size(indices), 1, 0, 0, 0);

  // issue a draw call
  // SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

  SDL_EndGPURenderPass(renderPass);

  SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void cleanup() {
  SDL_ReleaseGPUBuffer(device, vertex_buffer);
  SDL_ReleaseGPUBuffer(device, index_buffer);
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

  SDL_ReleaseGPUGraphicsPipeline(device, graphics_pipeline);

  SDL_DestroyGPUDevice(device);

  // SDL_DestroyTexture(texture);
  // SDL_DestroyRenderer(renderer);
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
  std::string toml_file_name = "./res/test.toml";
  std::string bitmap_write_name = "./res/test_image_write.bmp";
  std::string bitmap_read_name = "./res/test_image_read.bmp";

  std::ifstream toml_file(toml_file_name);
  if (!toml_file) {
    SDL_Log("Could not open toml");
    return 1;
  }
  std::string line;

  while (std::getline(toml_file, line)) {
    // Handle comments
    int comment_pos = line.find("#");
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }
    // Handle tables
    // Handle key-value pairs
    int equal_sign_pos = line.find("=");
    if (equal_sign_pos == std::string::npos) {
      continue;
    }
    std::string key = strip_lr(line.substr(0, equal_sign_pos));
    std::string raw_value =
        strip_lr(line.substr(equal_sign_pos + 1, line.length()));
    // Boolean true
    if (raw_value == "true") {
    }
    // Boolean false
    if (raw_value == "false") {
    }
    // Float
    if (raw_value.find(".") != std::string::npos) {
    }
    // vec2
    if (raw_value.starts_with("vec2")) {
    }
    std::cout << key << ":" << raw_value << std::endl;
  }

  int bitmap_write_width = 16;
  int bitmap_write_height = 16;

  // Create a test image (gradient)
  Bitmap bmp_write(bitmap_write_width, bitmap_write_height);

  for (int y = 0; y < bitmap_write_height; ++y) {
    for (int x = 0; x < bitmap_write_width; ++x) {
      uint8_t r =
          static_cast<int>(static_cast<float>(x) / bitmap_write_width * 255.0f);
      uint8_t g = static_cast<int>(static_cast<float>(y) / bitmap_write_height *
                                   255.0f);
      uint8_t b = 0;
      bmp_write.set_pixel(x, y, (r << 24) | (g << 16) | (b << 8) | 0xFF);
    }
  }

  // Dump BMP file
  if (!bmp_write.dump(bitmap_write_name)) {
    std::cerr << "Failed to write BMP file.\n";
    return 1;
  }

  // Read BMP file using constructor
  Bitmap bmp_read(bitmap_read_name);

  int bitmap_read_width = bmp_read.get_width();
  int bitmap_read_height = bmp_read.get_height();

  std::cout << "Width: " << bitmap_read_width << std::endl;
  std::cout << "Height: " << bitmap_read_height << std::endl;

  int success = init();

  if (success != 0) {
    return success;
  }

  EntityManager entity_manager;

  // Cursor
  EntityID cursor = entity_manager.create();
  SpriteComponent cursor_sprite = {
      .bitmap = bmp_write,
      .size = vec2(bmp_write.get_width(), bmp_write.get_height())};
  sprite_components[cursor] = cursor_sprite;
  TransformComponent cursor_transform;
  transform_components[cursor] = cursor_transform;

  // Player Amogus
  EntityID amogus = entity_manager.create();
  SpriteComponent sprite_component = {
      .bitmap = bmp_read,
      .size = vec2(bitmap_read_width, bitmap_read_height),
  };
  sprite_components[amogus] = sprite_component;
  TransformComponent transform_component = {
      .position = vec2(64.0f, 64.0f),
  };
  transform_components[amogus] = transform_component;
  RigidBodyComponent rigidbody_component = {
      .collision_aabb = AABB(vec2(-8.0f, -8.0f), vec2(8.0f, 8.0f)),
  };
  rigidbody_components[amogus] = rigidbody_component;

  std::mt19937 random_engine;
  std::uniform_real_distribution<float> velocity_distribution(-1.0f, 1.0f);
  std::uniform_real_distribution<float> position_distribution(0.0f, 1.0f);
  float velocity_magnitude = 512.0f;
  int friends = 16;
  for (int i = 0; i < friends; ++i) {
    EntityID amogus2 = entity_manager.create();
    SpriteComponent sprite_component2 = {
        .bitmap = bmp_read,
        .size = vec2(bitmap_read_width, bitmap_read_height)};
    sprite_components[amogus2] = sprite_component2;
    TransformComponent transform_component2;
    transform_component2.position =
        vec2(16.0f + (static_cast<float>(WIDTH) - 32.0f) *
                         position_distribution(random_engine),
             16.0f + (static_cast<float>(HEIGHT) - 32.0f) *
                         position_distribution(random_engine));
    transform_components[amogus2] = transform_component2;
    RigidBodyComponent rigidbody_component2;
    rigidbody_component2.collision_aabb =
        AABB(vec2(-8.0f, -8.0f), vec2(8.0f, 8.0f));
    rigidbody_component2.velocity =
        vec2(velocity_magnitude * velocity_distribution(random_engine),
             velocity_magnitude * velocity_distribution(random_engine));
    rigidbody_components[amogus2] = rigidbody_component2;
  }
  EntityID top_wall = entity_manager.create();
  StaticBodyComponent top_wall_staticbody_component;
  TransformComponent top_wall_transform_component;
  top_wall_transform_component.position = vec2(WIDTH / 2.0f, -8.0f);
  top_wall_staticbody_component.collision_aabb =
      AABB(vec2(-WIDTH / 2.0f, -8.0f), vec2(WIDTH / 2.0f, 8.0f));
  staticbody_components[top_wall] = top_wall_staticbody_component;
  transform_components[top_wall] = top_wall_transform_component;

  EntityID bottom_wall = entity_manager.create();
  StaticBodyComponent bottom_wall_staticbody_component;
  TransformComponent bottom_wall_transform_component;
  bottom_wall_transform_component.position = vec2(WIDTH / 2.0f, HEIGHT + 8.0f);
  bottom_wall_staticbody_component.collision_aabb =
      AABB(vec2(-WIDTH / 2.0f, -8.0f), vec2(WIDTH / 2.0f, 8.0f));
  staticbody_components[bottom_wall] = bottom_wall_staticbody_component;
  transform_components[bottom_wall] = bottom_wall_transform_component;

  EntityID left_wall = entity_manager.create();
  StaticBodyComponent left_wall_staticbody_component;
  TransformComponent left_wall_transform_component;
  left_wall_transform_component.position = vec2(-8.0f, HEIGHT / 2.0f);
  left_wall_staticbody_component.collision_aabb =
      AABB(vec2(-8.0f, -HEIGHT / 2.0f), vec2(8.0f, HEIGHT / 2.0f));
  staticbody_components[left_wall] = left_wall_staticbody_component;
  transform_components[left_wall] = left_wall_transform_component;

  EntityID right_wall = entity_manager.create();
  StaticBodyComponent right_wall_staticbody_component;
  TransformComponent right_wall_transform_component;
  right_wall_transform_component.position = vec2(WIDTH + 8.0f, HEIGHT / 2.0f);
  right_wall_staticbody_component.collision_aabb =
      AABB(vec2(-8.0f, -HEIGHT / 2.0f), vec2(8.0f, HEIGHT / 2.0f));
  staticbody_components[right_wall] = right_wall_staticbody_component;
  transform_components[right_wall] = right_wall_transform_component;

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
      TransformComponent &amogus_transform = transform_components[amogus];
      RigidBodyComponent &amogus_rigidbody = rigidbody_components[amogus];

      // Player specific code
      // WASD Movement
      float accel = 512.0f;
      vec2 move_dir = vec2(0.0f, 0.0f);
      if (keystate[SDL_SCANCODE_W]) {
        move_dir.y -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_A]) {
        move_dir.x -= 1.0f;
      }
      if (keystate[SDL_SCANCODE_S]) {
        move_dir.y += 1.0f;
      }
      if (keystate[SDL_SCANCODE_D]) {
        move_dir.x += 1.0f;
      }
      amogus_rigidbody.velocity += move_dir * accel * physics_delta_time;

      // Showcase rotation
      amogus_transform.rotation += 2.0f * physics_delta_time;

      // Physics tick-rate ECS system
      PhysicsSystem::Update(physics_delta_time);
    }
    float cursor_x;
    float cursor_y;

    SDL_GetMouseState(&cursor_x, &cursor_y);

    TransformComponent &cursorTransform = transform_components[cursor];
    cursorTransform.position = vec2(cursor_x, cursor_y) / scale;

    // --- Software rendering: fill the pixel buffer (checkerboard) ---
    int checker_size = 1;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        int checker = ((x / checker_size) + (y / checker_size)) % 2;
        uint8_t v = checker ? 192 : 128;
        back_buffer[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
      }
    }

    // Process tick-rate ECS sytems
    SpriteSystem::update_aabbs();
    SpriteSystem::draw_all(back_buffer, WIDTH, HEIGHT);

    loop();
  }
  cleanup();
  return 0;
}
