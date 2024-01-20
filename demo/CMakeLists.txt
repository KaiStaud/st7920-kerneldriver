cmake_minimum_required(VERSION 3.12)
project(IoctlDemo)

# Find the CLI11 library (assuming it's installed or available)
find_package(GTest REQUIRED)
# Add the executable target
add_executable(ioctl_demo main.cpp bmp-parser.cpp emulator.cpp)

enable_testing()

add_executable(
  glcdctrl-test
  bmp-parser_test.cpp
  bmp-parser.cpp
)
set_property(TARGET glcdctrl-test PROPERTY CXX_STANDARD 11)


target_link_libraries(
  glcdctrl-test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(glcdctrl-test)
