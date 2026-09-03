# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception Copyright (c) MQSS
# Maintainers

# CMake 3.28+ is required for FetchContent_Declare(EXCLUDE_FROM_ALL) used by
# SimpleAmqpClient integration.
cmake_minimum_required(VERSION 3.28)
project(mqss_integration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Protocol
include(cmake/protobuf.cmake)

add_library(mqss_protocol STATIC)
mqss_configure_proto_target(mqss_protocol)

# Transport
include(cmake/transport.cmake)

mqss_add_transport(inmemory SOURCES src/transport/InMemoryTransport.cpp)

option(MQSS_ENABLE_RABBITMQ_TRANSPORT "Build RabbitMQ transport" ON)

if(MQSS_ENABLE_RABBITMQ_TRANSPORT)
  include(cmake/SimpleAmqpClient.cmake)

  mqss_add_transport(
    rabbitmq_simple SOURCES src/transport/RabbitMqSimpleTransport.cpp
    PRIVATE_LIBS SimpleAmqpClient)
endif()

# Public aggregate target
add_library(mqss INTERFACE)
add_library(mqss::mqss ALIAS mqss)

target_link_libraries(mqss INTERFACE mqss_protocol mqss_transport_inmemory)

if(MQSS_ENABLE_RABBITMQ_TRANSPORT)
  target_link_libraries(mqss INTERFACE mqss_transport_rabbitmq_simple)
endif()

target_include_directories(
  mqss INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                 $<INSTALL_INTERFACE:include>)

target_compile_features(mqss INTERFACE cxx_std_23)

# Install/package export
include(cmake/install.cmake)

# Tests
include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
