cmake_minimum_required(VERSION 3.16)
project(voidline VERSION 2.10.23 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/voidline)
add_compile_definitions(VOIDLINE_VERSION="${PROJECT_VERSION}")

find_package(Threads REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Link-time optimization lets the compiler inline across the game, renderer and input sources.
include(CheckIPOSupported)
check_ipo_supported(RESULT VOIDLINE_IPO OUTPUT VOIDLINE_IPO_MESSAGE LANGUAGES CXX)
option(VOIDLINE_LTO "Enable link-time optimization when the toolchain supports it" ON)

if(WIN32)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The Windows compatibility build requires a 64-bit toolchain.")
  endif()
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
  find_package(SDL2 CONFIG REQUIRED)
  find_package(WebP CONFIG REQUIRED)
  find_package(PNG REQUIRED)
  find_package(SndFile CONFIG REQUIRED)
  find_package(json-c CONFIG REQUIRED)
  if(TARGET SDL2::SDL2)
    set(VOIDLINE_SDL SDL2::SDL2)
  else()
    set(VOIDLINE_SDL SDL2::SDL2-static)
  endif()
  if(TARGET json-c::json-c)
    set(VOIDLINE_JSON json-c::json-c)
  else()
    set(VOIDLINE_JSON json-c::json-c-static)
  endif()
  set(VOIDLINE_RUNTIME_LIBS ${VOIDLINE_SDL} WebP::webp PNG::PNG SndFile::sndfile ${VOIDLINE_JSON}
    ws2_32 iphlpapi bcrypt shell32 ole32 uuid)
  if(MSVC)
    set(VOIDLINE_WARNINGS /W3 /utf-8 /EHsc)
  endif()
else()
  # Linux uses the installed runtime libraries, without development packages.
  set(VOIDLINE_RUNTIME_LIBS
    -l:libSDL2-2.0.so.0 -l:libwebp.so.7 -l:libpng16.so.16 -l:libsndfile.so.1 -l:libjson-c.so.5)
endif()
if(NOT MSVC)
  set(VOIDLINE_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual)
  # Match MSVC C2397: braced int-to-float narrowing is a build error on all platforms.
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND VOIDLINE_WARNINGS -Werror=narrowing)
  endif()
endif()

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
voidline_module(voidline_services ${CMAKE_SOURCE_DIR}/src/platform/paths.cpp ${CMAKE_SOURCE_DIR}/src/net/*.cpp ${CMAKE_SOURCE_DIR}/src/services/*.cpp)
voidline_module(voidline_render ${CMAKE_SOURCE_DIR}/src/render/*.cpp ${CMAKE_SOURCE_DIR}/src/platform/gl_loader.cpp)
voidline_module(voidline_ui ${CMAKE_SOURCE_DIR}/src/ui/*.cpp)
target_link_libraries(voidline_services PUBLIC voidline_game)
target_link_libraries(voidline_render PUBLIC voidline_game)
target_link_libraries(voidline_ui PUBLIC voidline_render voidline_services)

file(GLOB VOIDLINE_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/engine/*.cpp)
add_executable(voidline src/main.cpp ${VOIDLINE_ENGINE_SOURCES})
target_include_directories(voidline PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(voidline PRIVATE ${VOIDLINE_WARNINGS})
target_link_libraries(voidline PRIVATE voidline_ui)
if(WIN32)
  target_link_libraries(voidline PRIVATE d3d12 dxgi dxguid)
  if(MINGW)
    target_link_options(voidline PRIVATE -municode)
  endif()
endif()
# The packaged runtime never contains individual images or sound recordings.
file(GLOB_RECURSE VOIDLINE_PACK_INPUTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/assets/*)
set(VOIDLINE_ASSET_PACK ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/game.vpak)
add_custom_command(OUTPUT ${VOIDLINE_ASSET_PACK}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/pack-assets.py
          ${CMAKE_SOURCE_DIR}/assets ${VOIDLINE_ASSET_PACK}
  DEPENDS ${VOIDLINE_PACK_INPUTS} ${CMAKE_SOURCE_DIR}/scripts/pack-assets.py
  COMMENT "Bundling artwork and audio into game.vpak"
  VERBATIM)
add_custom_target(voidline_media ALL DEPENDS ${VOIDLINE_ASSET_PACK})
add_dependencies(voidline voidline_media)
# Editable stages are loose JSON read beside the executable by Game::start.
# Publish Models+Levels writes only changed overrides to source/levels;
# sync each build to Linux and Windows runtime paths, including removals.
add_custom_target(voidline_levels ALL
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/sync-levels.py
          ${CMAKE_SOURCE_DIR}/levels $<TARGET_FILE_DIR:voidline>/levels
  DEPENDS voidline ${CMAKE_SOURCE_DIR}/scripts/sync-levels.py
  COMMENT "Synchronizing authored level overrides"
  VERBATIM)
# Visual Studio multi-configuration builds place the EXE under voidline/Release.
# Stage the media beside the actual executable on Windows as well as Linux.
if(WIN32)
  add_custom_command(TARGET voidline POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:voidline>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VOIDLINE_ASSET_PACK} $<TARGET_FILE_DIR:voidline>/assets/game.vpak
    VERBATIM)
endif()

# Linux-only model authoring tool. It has its own version and workspace; it is
# built alongside the game but never linked to VOIDLINE or staged into its runtime.
# The Windows build ignores this entire directory, including its dependencies.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "${CMAKE_SOURCE_DIR}/model-studio/CMakeLists.txt")
  option(VOIDLINE_BUILD_MODEL_STUDIO "Build native Linux Model Studio with VOIDLINE" ON)
  if(VOIDLINE_BUILD_MODEL_STUDIO)
    add_subdirectory(model-studio)
  endif()
endif()

# Authoritative editable model JSON lives in source/models and is published with Model Studio. Only game.vpak
# (including the baked VLRM) is needed by deployed game clients.

if(VOIDLINE_LTO AND VOIDLINE_IPO)
  set_property(TARGET voidline ${VOIDLINE_MODULES} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
  set(VOIDLINE_LTO_STATE "enabled")
else()
  set(VOIDLINE_LTO_STATE "disabled")
endif()

install(TARGETS voidline RUNTIME DESTINATION voidline)
install(FILES ${VOIDLINE_ASSET_PACK} DESTINATION voidline/assets)
# CMake install is normally game-binary-only; stage any published level JSON.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/levels/ DESTINATION voidline/levels
        OPTIONAL FILES_MATCHING PATTERN "*.vllevel.json")

include(CTest)
if(BUILD_TESTING)
  file(GLOB VOIDLINE_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.cpp)
  add_executable(voidline_tests ${VOIDLINE_TEST_SOURCES})
  # Unit tests must not share the actual game's runtime level overrides.
  # Model Studio publishes edited levels into voidline/levels; co-locating the
  # test executable with the game makes built-in level assertions depend on
  # user-authored content (tests/test_progression.cpp).
  set_target_properties(voidline_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test-runner")
  target_compile_options(voidline_tests PRIVATE ${VOIDLINE_WARNINGS})
  target_link_libraries(voidline_tests PRIVATE voidline_services)
  add_dependencies(voidline_tests voidline_media)
  target_compile_definitions(voidline_tests PRIVATE VOIDLINE_TEST_ASSET_ROOT="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    VOIDLINE_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_test(NAME voidline_tests COMMAND voidline_tests)
  add_test(NAME voidline_authored_levels
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/test_authored_levels.py)
  add_test(NAME voidline_level_sync
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/test_sync_levels.py)
  # GUI logic regression checks are headless, so CI and ./build.sh exercise them
  # without a running X server. Windows never includes Model Studio.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "${CMAKE_SOURCE_DIR}/model-studio/ui/test_clipboard_dialogs.py")
    add_test(NAME voidline_model_studio_python
      COMMAND ${Python3_EXECUTABLE} -m unittest discover
              -s ${CMAKE_SOURCE_DIR}/model-studio/ui -p test_*.py -v)
  endif()
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
message(STATUS "  Runtime libs    : ${VOIDLINE_RUNTIME_LIBS}")
message(STATUS "  Tests           : ${BUILD_TESTING}")
message(STATUS "  Output          : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "")
