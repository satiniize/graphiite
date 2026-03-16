add_library(stb STATIC src/stb_image.cpp src/stb_truetype.cpp)
target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
