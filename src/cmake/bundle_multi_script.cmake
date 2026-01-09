# Generates AviUtl2 .obj2 (multi-object script) from multiple Lua files.
#
# Inputs:
#   -Dinput_dir=<dir>           Directory containing .lua files to embed (recursively).
#   -Doutput_file=<path>        Output .obj2 path.
#   -Dversion_env_file=<path>   (optional) Path to version.env file to read version info from.

cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED input_dir OR input_dir STREQUAL "")
  message(FATAL_ERROR "input_dir is required")
endif()
if(NOT DEFINED output_file OR output_file STREQUAL "")
  message(FATAL_ERROR "output_file is required")
endif()

# Read version from version.env file if provided.
set(version "")
if(DEFINED version_env_file AND EXISTS "${version_env_file}")
  file(STRINGS "${version_env_file}" version_env_lines)
  foreach(line IN LISTS version_env_lines)
    if(line MATCHES "^PTK_VERSION=(.+)$")
      set(version "${CMAKE_MATCH_1}")
      break()
    endif()
  endforeach()
endif()

file(GLOB_RECURSE obj2_lua_files LIST_DIRECTORIES false "${input_dir}/*.lua")
list(SORT obj2_lua_files)

# Define a real newline character for string concatenation.
string(ASCII 10 LF)

set(obj2_out "")

foreach(lua_file IN LISTS obj2_lua_files)
  file(READ "${lua_file}" lua_content)

  # Normalize newlines for stable parsing.
  string(REPLACE "\r\n" "\n" lua_content "${lua_content}")
  string(REPLACE "\r" "\n" lua_content "${lua_content}")

  # Strip UTF-8 BOM if present (EF BB BF).
  # CMake regex does not support octal/hex escapes, so we check via string length.
  string(LENGTH "${lua_content}" content_len)
  if(content_len GREATER_EQUAL 3)
    string(SUBSTRING "${lua_content}" 0 3 maybe_bom)
    # Compare against literal BOM bytes using file(STRINGS) trick is complex,
    # so we use ASCII code points: BOM = 0xEF 0xBB 0xBF
    # We check if first char code is 239 (0xEF)
    string(SUBSTRING "${lua_content}" 0 1 first_char)
    # CMake has no ord() function. Instead, just skip BOM removal for now
    # since source Lua files are expected to be UTF-8 without BOM.
  endif()

  # Split into first line and body using string(FIND) to avoid regex issues with multibyte chars.
  string(FIND "${lua_content}" "\n" first_newline_pos)
  if(first_newline_pos EQUAL -1)
    # No newline found, entire content is first line
    set(first_line "${lua_content}")
    set(body "")
  else()
    string(SUBSTRING "${lua_content}" 0 ${first_newline_pos} first_line)
    math(EXPR body_start "${first_newline_pos} + 1")
    string(SUBSTRING "${lua_content}" ${body_start} -1 body)
  endif()

  set(obj_name "")
  set(obj_body "")

  if(first_line MATCHES "^--@(.+)$")
    set(obj_name "${CMAKE_MATCH_1}")
    set(obj_body "${body}")
  else()
    get_filename_component(base_name "${lua_file}" NAME_WE)
    set(obj_name "${base_name}")
    set(obj_body "${lua_content}")
  endif()

  string(STRIP "${obj_name}" obj_name)
  if(obj_name STREQUAL "")
    get_filename_component(base_name "${lua_file}" NAME_WE)
    set(obj_name "${base_name}")
  endif()

  # Ensure body ends with newline.
  if(NOT obj_body MATCHES "\n$")
    set(obj_body "${obj_body}${LF}")
  endif()

  # Emit one object.
  string(CONCAT obj2_out "${obj2_out}" "@${obj_name}${LF}" "${obj_body}" "${LF}")
endforeach()

# Replace %VERSION% with actual version string if provided.
if(DEFINED version AND NOT version STREQUAL "")
  string(REPLACE "%VERSION%" "${version}" obj2_out "${obj2_out}")
endif()

file(WRITE "${output_file}" "${obj2_out}")
