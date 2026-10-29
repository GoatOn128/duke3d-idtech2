cmake_minimum_required(VERSION 3.24)
project(
  Duke3D
  VERSION 0.1
  LANGUAGES C CXX RC)

if(NOT WIN32)
  message(FATAL_ERROR "The platform layer currently requires Windows.")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(
    FATAL_ERROR
      "Duke3D requires Clang. Run 'call tools/vsenv.cmd x86' in a console, "
      "then configure the 'windows-clang-x86' preset from code/CMakePresets.json.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Repository layout shared by every project file under cmake/.
get_filename_component(DUKE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(DUKE_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DUKE_THIRD_PARTY_DIR "${DUKE_CODE_DIR}/third-party")

# The engine and its game modules target x86 by default (dngamex86/cgamex86).
# Pass -DDN_ARCH=x64 to build 64-bit instead; the vendored OpenAL libs exist
# for both architectures.
set(DN_ARCH "x86" CACHE STRING "Target architecture: x86 (default) or x64")

include(cmake/utils.cmake)
duke_apply_global_compile_settings()
duke_check_target_architecture()

# One file per project. They are included (not add_subdirectory) so that
# CMAKE_CURRENT_SOURCE_DIR stays at code/ and every source path stays
# relative to this directory.
include(cmake/third-party.cmake)
include(cmake/renderer.cmake)
include(cmake/game.cmake)
include(cmake/cgame.cmake)
include(cmake/ui.cmake)
include(cmake/engine.cmake)
include(cmake/assets.cmake)
include(cmake/tests.cmake)
