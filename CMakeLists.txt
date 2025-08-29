cmake_minimum_required(VERSION 3.16)
project(software-renderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(LIBJPEG_TURBO_INSTALL_DIR "${CMAKE_BINARY_DIR}/libjpeg-turbo-install")

include(ExternalProject)
ExternalProject_Add(libjpeg_turbo
		SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/libjpeg-turbo
		CMAKE_ARGS
				-DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
				-DENABLE_SHARED=OFF
				-DENABLE_STATIC=ON
				-DWITH_SIMD=ON
)

include_directories(${LIBJPEG_TURBO_INSTALL_DIR}/include)
link_directories(${LIBJPEG_TURBO_INSTALL_DIR}/lib)

add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL_ttf EXCLUDE_FROM_ALL)

set(SR_SOURCES
		src/main.cpp
		src/sprite_system.cpp
		src/renderer.cpp
		src/clay_renderer.cpp
)

add_executable(software-renderer ${SR_SOURCES})
add_dependencies(software-renderer libjpeg_turbo)

target_include_directories(software-renderer PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		"${LIBJPEG_TURBO_INSTALL_DIR}/include"
)

target_link_libraries(software-renderer PRIVATE
		SDL3::SDL3
		SDL3_image::SDL3_image
		SDL3_ttf::SDL3_ttf
		glm::glm
		"${LIBJPEG_TURBO_INSTALL_DIR}/lib/libjpeg.a"
		"${LIBJPEG_TURBO_INSTALL_DIR}/lib/libturbojpeg.a"
)
