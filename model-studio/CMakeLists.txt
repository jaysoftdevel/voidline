cmake_minimum_required(VERSION 3.16)
project(voidline_model_studio VERSION 0.0.16 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "VOIDLINE Model Studio is Linux-only")
endif()
find_package(Threads REQUIRED)
set(IMAGELIBS -l:libwebp.so.7 -l:libpng16.so.16)
set(JSONLIB -l:libjson-c.so.5)
add_executable(voidline-editor
  editor/cli.cpp
  editor/art.cpp
  editor/workspace.cpp
  editor/source_export.cpp
  src/content/stages.cpp
  src/content/cooperative.cpp
  src/content/level_io.cpp
  src/content/model_io.cpp)
# The C++ process is the tested, headless import/export engine. GUI uses Tk/ttk.
set_target_properties(voidline-editor PROPERTIES OUTPUT_NAME "voidline-editor-core")
# Integrated game builds stage the editor outside the shipped game runtime.
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set_target_properties(voidline-editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/model-studio")
endif()
target_include_directories(voidline-editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(voidline-editor PRIVATE ${JSONLIB} ${IMAGELIBS} Threads::Threads)
if(MSVC)
  target_compile_options(voidline-editor PRIVATE /W3 /utf-8 /EHsc)
else()
  target_compile_options(voidline-editor PRIVATE -Wall -Wextra -Wpedantic -Werror=narrowing)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(voidline-editor-file-tests
    tests/file_roundtrip.cpp
    editor/art.cpp
    editor/workspace.cpp
    src/content/stages.cpp
    src/content/cooperative.cpp
    src/content/level_io.cpp
    src/content/model_io.cpp)
  target_include_directories(voidline-editor-file-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(voidline-editor-file-tests PRIVATE ${JSONLIB} ${IMAGELIBS} Threads::Threads)
  if(MSVC)
    target_compile_options(voidline-editor-file-tests PRIVATE /W3 /utf-8 /EHsc)
  else()
    target_compile_options(voidline-editor-file-tests PRIVATE -Wall -Wextra -Wpedantic -Werror=narrowing)
  endif()
  add_test(NAME voidline-editor-file-roundtrip COMMAND voidline-editor-file-tests)
endif()
