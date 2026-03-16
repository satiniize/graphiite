add_library(tinyfiledialogs STATIC src/tinyfiledialogs.c)
target_include_directories(tinyfiledialogs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
