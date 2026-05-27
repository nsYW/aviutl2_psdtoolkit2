cmake_minimum_required(VERSION 3.10)
project(psdtoolkit C)
enable_testing()

set(PTK_REQUIRED_AVIUTL2_VERSION "2.0beta44")
set(PTK_REQUIRED_GCMZDROPS_VERSION_TAG "v2.0.0beta2")
include(src/cmake/requirements.cmake)

set(EXPORT_DIR_NAME "bin" CACHE STRING "export directory")
if ("${EXPORT_DIR_NAME}" STREQUAL "")
  set(EXPORT_DIR_NAME "bin" CACHE STRING "export directory" FORCE)
endif()
set(EXPORT_DIR "${PROJECT_BINARY_DIR}/${EXPORT_DIR_NAME}")

add_subdirectory(src/c)
add_subdirectory(src/go)
