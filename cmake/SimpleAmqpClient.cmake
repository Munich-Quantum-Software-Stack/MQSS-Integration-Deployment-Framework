# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

include(FetchContent)

find_package(Git REQUIRED)

# Disable optional upstream features
set(BUILD_API_DOCS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  simple_amqp_client
  GIT_REPOSITORY https://github.com/alanxz/SimpleAmqpClient.git
  GIT_TAG v2.5.1
  GIT_SUBMODULES ""
  PATCH_COMMAND
    "${GIT_EXECUTABLE}" apply --ignore-whitespace
      "${CMAKE_CURRENT_SOURCE_DIR}/patches/simple-amqp-client-v2.5.1.patch"
)

FetchContent_MakeAvailable(simple_amqp_client)

# Fix include path export (missing upstream)
target_include_directories(SimpleAmqpClient
  PUBLIC
    "${simple_amqp_client_SOURCE_DIR}/src"
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
