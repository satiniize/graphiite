cmake_minimum_required(VERSION 3.16)
project(software-renderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# libjpeg-turbo shenanigans
set(LIBJPEG_TURBO_PREFIX "${CMAKE_BINARY_DIR}/external/libjpeg_turbo")
set(LIBJPEG_TURBO_INSTALL_DIR "${LIBJPEG_TURBO_PREFIX}/install")

include(ExternalProject)

if(MSVC)
    set(LIB_JPEG_NAME "jpeg-static.lib")
    set(LIB_TURBO_NAME "turbojpeg-static.lib")
else()
    set(LIB_JPEG_NAME "libjpeg.a")
    set(LIB_TURBO_NAME "libturbojpeg.a")
endif()

ExternalProject_Add(libjpeg_turbo_project
    PREFIX "${LIBJPEG_TURBO_PREFIX}"
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/libjpeg-turbo
    INSTALL_DIR "${LIBJPEG_TURBO_INSTALL_DIR}"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DWITH_SIMD=ON
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    BUILD_BYPRODUCTS
        "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_TURBO_NAME}"
        "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_JPEG_NAME}"
)

add_library(libjpeg_turbo STATIC IMPORTED GLOBAL)
file(MAKE_DIRECTORY "${LIBJPEG_TURBO_INSTALL_DIR}/include")
set_target_properties(libjpeg_turbo PROPERTIES
    IMPORTED_LOCATION "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_TURBO_NAME}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBJPEG_TURBO_INSTALL_DIR}/include"
    INTERFACE_LINK_LIBRARIES "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${LIB_JPEG_NAME}"
)

add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL_ttf EXCLUDE_FROM_ALL)

set(SR_SOURCES
  src/main.cpp
  src/sprite_system.cpp
  src/renderer.cpp
  src/clay_renderer.cpp
  src/tinyfiledialogs.c
)

add_executable(software-renderer ${SR_SOURCES})
add_dependencies(software-renderer libjpeg_turbo)

target_include_directories(software-renderer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  lib/glm
)

target_link_libraries(software-renderer PRIVATE
  SDL3::SDL3
  SDL3_image::SDL3_image
  SDL3_ttf::SDL3_ttf
  libjpeg_turbo
)
