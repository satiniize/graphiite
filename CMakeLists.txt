cmake_minimum_required(VERSION 3.16)
project(graphiite)

# Global settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# SDL static build
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

# libjpeg-turbo (ExternalProjects)
include(ExternalProject)

set(LIBJPEG_TURBO_PREFIX "${CMAKE_BINARY_DIR}/external/libjpeg_turbo")
set(LIBJPEG_TURBO_INSTALL_DIR "${LIBJPEG_TURBO_PREFIX}/install")

if(MSVC)
  set(LIB_JPEG_NAME "jpeg-static.lib")
  set(LIB_TURBO_NAME "turbojpeg-static.lib")
else()
  set(LIB_JPEG_NAME "libjpeg.a")
  set(LIB_TURBO_NAME "libturbojpeg.a")
endif()

ExternalProject_Add(libjpeg_turbo_project
    PREFIX "${LIBJPEG_TURBO_PREFIX}"
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/libjpeg-turbo
    INSTALL_DIR "${LIBJPEG_TURBO_INSTALL_DIR}"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DWITH_SIMD=ON
    BUILD_BYPRODUCTS
        "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_TURBO_NAME}"
        "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_JPEG_NAME}"
)

# Windows missing directory workaround
file(MAKE_DIRECTORY "${LIBJPEG_TURBO_INSTALL_DIR}/include")

add_library(libjpeg_turbo STATIC IMPORTED GLOBAL)
add_dependencies(libjpeg_turbo libjpeg_turbo_project)
set_target_properties(libjpeg_turbo PROPERTIES
    IMPORTED_LOCATION "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_TURBO_NAME}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBJPEG_TURBO_INSTALL_DIR}/include"
    INTERFACE_LINK_LIBRARIES "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_JPEG_NAME}"
)

# External dependencies
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/glm EXCLUDE_FROM_ALL)
add_subdirectory(external/stb EXCLUDE_FROM_ALL)
add_subdirectory(external/clay EXCLUDE_FROM_ALL)
add_subdirectory(external/tinyfiledialogs EXCLUDE_FROM_ALL)
add_subdirectory(external/LibRaw EXCLUDE_FROM_ALL)

add_library(graphiite STATIC
  src/base_components.cpp
  src/clay_manager.cpp
  src/clay_renderer.cpp
  src/file_dialog.cpp
  src/image.cpp
  src/image_io.cpp
  src/input_manager.cpp
  src/renderer.cpp
  src/sprite_renderer.cpp
)

# TODO: Probably delete this after testing on windows
add_dependencies(graphiite libjpeg_turbo)
add_dependencies(graphiite libjpeg_turbo_project)

target_include_directories(graphiite PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(graphiite
PUBLIC
  SDL3::SDL3
  glm::glm
  clay
PRIVATE
  stb
  tinyfiledialogs
  libjpeg_turbo
)

add_subdirectory(examples/oxiide)
add_subdirectory(tests)
