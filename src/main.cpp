#include <SDL3/SDL.h>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#define WIDTH 320
#define HEIGHT 180

#include "bitmap.h"
#include "vec2.h"

#include "config.h"

// Entities
#include "component_storage.h"
#include "entity_manager.h"

// Systems
#include "physics_system.h"
#include "sprite_system.h"

// Components
#include "rigidbody_component.h"
#include "sprite_component.h"
#include "transform_component.h"

SDL_Window *window;
SDL_GPUDevice *device;
SDL_Renderer *renderer;
SDL_Texture *texture;

const int scale = 4;

// Software buffer for screen pixels
uint32_t back_buffer[WIDTH * HEIGHT];

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

  // SDL_GPU
  device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  SDL_ClaimWindowForGPUDevice(device, window);

  SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);

  SDL_SubmitGPUCommandBuffer(commandBuffer);

  // SDL_GPUTextureCreateInfo *gpu_texture_create_info;

  // SDL_GPUTexture *gpu_texture =
  //     SDL_CreateGPUTexture(device, gpu_texture_create_info);

  renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_Log("Couldn't create renderer: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  if (!texture) {
    SDL_Log("Couldn't create texture: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  return 0;
}

void loop() {
  // Upload pixel buffer to texture
  void *front_buffer;
  int pitch;
  SDL_LockTexture(texture, NULL, &front_buffer, &pitch);
  memcpy(front_buffer, back_buffer, WIDTH * HEIGHT * sizeof(uint32_t));
  SDL_UnlockTexture(texture);

  // --- Render the texture to the window ---
  SDL_RenderClear(renderer);
  SDL_FRect dst = {0, 0, WIDTH * scale, HEIGHT * scale};
  SDL_RenderTexture(renderer, texture, NULL, &dst);
  SDL_RenderPresent(renderer);
}

void cleanup() {
  SDL_DestroyGPUDevice(device);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
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
    int comment_pos = line.find("#");
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }
    int equal_sign_pos = line.find("=");
    if (equal_sign_pos == std::string::npos) {
      continue;
    }
    std::string key = line.substr(0, equal_sign_pos);
    std::string value = line.substr(equal_sign_pos + 1, line.length());
    std::cout << key << ": " << value << std::endl;
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
