# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

include(FetchContent)

find_package(Git REQUIRED)

# Disable optional upstream features
set(BUILD_API_DOCS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  simple_amqp_client
  EXCLUDE_FROM_ALL
  GIT_REPOSITORY https://github.com/alanxz/SimpleAmqpClient.git
  GIT_TAG v2.5.1
  GIT_SUBMODULES ""
  PATCH_COMMAND
  "${CMAKE_COMMAND}"
    "-DGIT_EXECUTABLE=${GIT_EXECUTABLE}"
    "-DSOURCE_DIR=<SOURCE_DIR>"
    "-DPATCH_FILE=${CMAKE_CURRENT_SOURCE_DIR}/patches/simple-amqp-client-v2.5.1.patch"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patch_once.cmake"
)

FetchContent_MakeAvailable(simple_amqp_client)

# Prevent exporting upstream dependencies (e.g., Boost)
set_target_properties(SimpleAmqpClient PROPERTIES
  INTERFACE_LINK_LIBRARIES ""
)

# Expose upstream headers for local build
target_include_directories(SimpleAmqpClient
  PUBLIC
    $<BUILD_INTERFACE:${simple_amqp_client_SOURCE_DIR}/src>
)

# Override old C++ standard (upstream uses C++98)
set_target_properties(SimpleAmqpClient PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

# Suppress noisy GCC warnings from Boost internals
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(SimpleAmqpClient PRIVATE
    -Wno-maybe-uninitialized
  )
endif()
