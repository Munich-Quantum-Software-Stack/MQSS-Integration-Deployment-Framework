# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

cmake_minimum_required(VERSION 3.24)
project(mqss_integration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Protocol
include(cmake/protobuf.cmake)

add_library(mqss_protocol STATIC)
mqss_configure_proto_target(mqss_protocol)

# Transport
include(cmake/transport.cmake)

mqss_add_transport(inmemory
  SOURCES
    src/transport/InMemoryTransport.cpp
)

# Tests
include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()