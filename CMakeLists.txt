cmake_minimum_required(VERSION 3.16)
project(software-renderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set the output directory for built objects.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in lib/SDL (as a submodule)
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

# Add SDL_image as a submodule
add_subdirectory(lib/SDL_image EXCLUDE_FROM_ALL)

add_subdirectory(lib/glm EXCLUDE_FROM_ALL)

add_subdirectory(lib/SDL_ttf EXCLUDE_FROM_ALL)

# Source files for the project
set(SR_SOURCES
	src/main.cpp
	src/sprite_system.cpp
	src/renderer.cpp
	src/clay_renderer.cpp
	)

# Create your game executable target as usual
add_executable(software-renderer ${SR_SOURCES})
target_include_directories(software-renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to the actual SDL3 library.
target_link_libraries(software-renderer PRIVATE SDL3::SDL3)
# Link to SDL_image
target_link_libraries(software-renderer PRIVATE SDL3_image::SDL3_image)

target_link_libraries(software-renderer PRIVATE glm::glm)

target_link_libraries(software-renderer PRIVATE SDL3_ttf::SDL3_ttf)

target_link_libraries(software-renderer PRIVATE glm::glm)

target_link_libraries(software-renderer PRIVATE SDL3_ttf::SDL3_ttf)
