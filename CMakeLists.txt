cmake_minimum_required(VERSION 3.16)
project(voidline VERSION 2.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/voidline)
add_compile_definitions(VOIDLINE_VERSION="${PROJECT_VERSION}")

find_package(Threads REQUIRED)

# Link-time optimization lets the compiler inline across the game, renderer and input sources.
include(CheckIPOSupported)
check_ipo_supported(RESULT VOIDLINE_IPO OUTPUT VOIDLINE_IPO_MESSAGE LANGUAGES CXX)
option(VOIDLINE_LTO "Enable link-time optimization when the toolchain supports it" ON)

# The platform ABI is declared in src/platform.hpp, so only the runtime shared objects are required.
# Development packages are not needed. Install the runtime libraries with your package manager:
# SDL2, libwebp, libpng16, libsndfile and json-c.
set(VOIDLINE_RUNTIME_LIBS
  -l:libSDL2-2.0.so.0 -l:libwebp.so.7 -l:libpng16.so.16 -l:libsndfile.so.1 -l:libjson-c.so.5)

set(VOIDLINE_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual)

# The sources are split into modules with a one-way dependency order:
#   game, input  ->  services  ->  render  ->  ui  ->  engine (executable)
# Each module is a static library built from every .cpp file in its directories.
function(voidline_module name)
  file(GLOB_RECURSE module_sources CONFIGURE_DEPENDS ${ARGN})
  add_library(${name} STATIC ${module_sources})
  target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/src)
  target_compile_options(${name} PRIVATE ${VOIDLINE_WARNINGS})
  target_link_libraries(${name} PUBLIC Threads::Threads ${VOIDLINE_RUNTIME_LIBS})
  list(APPEND VOIDLINE_MODULES ${name})
  set(VOIDLINE_MODULES ${VOIDLINE_MODULES} PARENT_SCOPE)
endfunction()

voidline_module(voidline_game ${CMAKE_SOURCE_DIR}/src/content/*.cpp ${CMAKE_SOURCE_DIR}/src/game/*.cpp ${CMAKE_SOURCE_DIR}/src/input/*.cpp)
voidline_module(voidline_services ${CMAKE_SOURCE_DIR}/src/net/*.cpp ${CMAKE_SOURCE_DIR}/src/services/*.cpp)
voidline_module(voidline_render ${CMAKE_SOURCE_DIR}/src/render/*.cpp ${CMAKE_SOURCE_DIR}/src/platform/*.cpp)
voidline_module(voidline_ui ${CMAKE_SOURCE_DIR}/src/ui/*.cpp)
target_link_libraries(voidline_services PUBLIC voidline_game)
target_link_libraries(voidline_render PUBLIC voidline_game)
target_link_libraries(voidline_ui PUBLIC voidline_render voidline_services)

file(GLOB VOIDLINE_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/engine/*.cpp)
add_executable(voidline src/main.cpp ${VOIDLINE_ENGINE_SOURCES})
target_include_directories(voidline PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(voidline PRIVATE ${VOIDLINE_WARNINGS})
target_link_libraries(voidline PRIVATE voidline_ui)
add_custom_command(TARGET voidline POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:voidline>/assets)

if(VOIDLINE_LTO AND VOIDLINE_IPO)
  set_property(TARGET voidline ${VOIDLINE_MODULES} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
  set(VOIDLINE_LTO_STATE "enabled")
else()
  set(VOIDLINE_LTO_STATE "disabled")
endif()

install(TARGETS voidline RUNTIME DESTINATION voidline)
install(DIRECTORY assets DESTINATION voidline)

include(CTest)
if(BUILD_TESTING)
  file(GLOB VOIDLINE_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.cpp)
  add_executable(voidline_tests ${VOIDLINE_TEST_SOURCES})
  target_compile_options(voidline_tests PRIVATE ${VOIDLINE_WARNINGS})
  target_link_libraries(voidline_tests PRIVATE voidline_services)
  add_test(NAME voidline_tests COMMAND voidline_tests)
endif()

file(GLOB_RECURSE VOIDLINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.hpp)
file(GLOB_RECURSE VOIDLINE_ASSETS ${CMAKE_SOURCE_DIR}/assets/*)
list(LENGTH VOIDLINE_SOURCES VOIDLINE_SOURCE_COUNT)
list(LENGTH VOIDLINE_ASSETS VOIDLINE_ASSET_COUNT)
message(STATUS "")
message(STATUS "VOIDLINE ${PROJECT_VERSION} configuration")
message(STATUS "  Build type      : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")
message(STATUS "  C++ standard    : C++${CMAKE_CXX_STANDARD}")
string(TOUPPER "${CMAKE_BUILD_TYPE}" VOIDLINE_TYPE_UPPER)
message(STATUS "  Optimization    : ${CMAKE_CXX_FLAGS_${VOIDLINE_TYPE_UPPER}}")
message(STATUS "  Warnings        : ${VOIDLINE_WARNINGS}")
message(STATUS "  Link-time opt.  : ${VOIDLINE_LTO_STATE}")
message(STATUS "  Generator       : ${CMAKE_GENERATOR}")
message(STATUS "  Source files    : ${VOIDLINE_SOURCE_COUNT} in src/ (modules: game, services, render, ui, engine)")
message(STATUS "  Asset files     : ${VOIDLINE_ASSET_COUNT} in assets/")
message(STATUS "  Runtime libs    : SDL2, libwebp, libpng16, libsndfile, json-c (loaded by soname, no dev packages)")
message(STATUS "  Tests           : ${BUILD_TESTING}")
message(STATUS "  Output          : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "")
