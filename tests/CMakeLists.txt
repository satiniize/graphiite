include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(tests
  image_io_tests.cpp
)

target_link_libraries(tests
  PRIVATE
    GTest::gtest_main
    software-renderer  # your library target
    SDL3::SDL3
    libjpeg_turbo
    clay
    glm::glm
    stb
    libraw::libraw
    tinyfiledialogs
)

include(GoogleTest)
gtest_discover_tests(tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
