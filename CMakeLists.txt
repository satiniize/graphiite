cmake_minimum_required(VERSION 3.16)
project(software-renderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/libjpeg-turbo
    INSTALL_DIR "${LIBJPEG_TURBO_INSTALL_DIR}"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DWITH_SIMD=ON
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
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

set(BUILD_SHARED_LIBS OFF)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(external/glm EXCLUDE_FROM_ALL)
add_subdirectory(external/LibRaw EXCLUDE_FROM_ALL)
add_subdirectory(external/stb EXCLUDE_FROM_ALL)
add_subdirectory(external/tinyfiledialogs EXCLUDE_FROM_ALL)
add_subdirectory(external/clay EXCLUDE_FROM_ALL)

set(SR_SOURCES
  src/sprite_renderer.cpp
  src/renderer.cpp
  src/clay_renderer.cpp
  src/clay_manager.cpp
  src/input_manager.cpp
  src/base_components.cpp

  src/image_io.cpp
  src/image.cpp

  src/file_dialog.cpp
)

add_library(software-renderer STATIC ${SR_SOURCES})
add_dependencies(software-renderer libjpeg_turbo)

target_include_directories(software-renderer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(software-renderer
PUBLIC
  SDL3::SDL3
  clay
  glm::glm
PRIVATE
  stb
  tinyfiledialogs
  # This one is technically not vital for software renderer, as well as libraw
  libjpeg_turbo
)

add_subdirectory(examples/oxiide)
add_subdirectory(tests)

# add_subdirectory(examples/photo-sorter)
