cmake_minimum_required(VERSION 3.16)
project(voidline VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/voidline)

find_package(Threads REQUIRED)

# The platform ABI is declared in src/platform.hpp, so only the runtime shared objects are required.
# Development packages are not needed. Install the runtime libraries with your package manager:
# SDL2, libwebp, libpng16, libsndfile and json-c.
set(VOIDLINE_RUNTIME_LIBS
  -l:libSDL2-2.0.so.0 -l:libwebp.so.7 -l:libpng16.so.16 -l:libsndfile.so.1 -l:libjson-c.so.5)

add_library(voidline_core STATIC src/game.cpp src/input.cpp)
target_include_directories(voidline_core PUBLIC src)
target_compile_options(voidline_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(voidline_core PUBLIC Threads::Threads ${VOIDLINE_RUNTIME_LIBS})

add_executable(voidline
  src/main.cpp src/engine.cpp src/app.cpp src/renderer.cpp src/world.cpp src/figure.cpp src/services.cpp)
target_compile_options(voidline PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(voidline PRIVATE voidline_core)
add_custom_command(TARGET voidline POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:voidline>/assets)

install(TARGETS voidline RUNTIME DESTINATION voidline)
install(DIRECTORY assets DESTINATION voidline)

include(CTest)
if(BUILD_TESTING)
  add_executable(voidline_tests tests/tests.cpp)
  target_compile_options(voidline_tests PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(voidline_tests PRIVATE voidline_core)
  add_test(NAME voidline_tests COMMAND voidline_tests)
endif()
