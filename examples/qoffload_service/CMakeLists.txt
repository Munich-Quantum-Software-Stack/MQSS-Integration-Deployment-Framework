# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

cmake_minimum_required(VERSION 3.28)
project(qoffload_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(mqss REQUIRED)
find_package(spdlog REQUIRED)

include(Config.cmake)

configure_file(
  ConfigDefaults.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ConfigDefaults.hpp
  @ONLY
)

add_library(qoffload-core INTERFACE)
add_library(qoffload::core ALIAS qoffload-core)

target_include_directories(
  qoffload-core
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

target_link_libraries(qoffload-core INTERFACE mqss::mqss spdlog::spdlog)

target_compile_options(
  qoffload-core
  INTERFACE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(qoffload src/main.cpp)
target_link_libraries(qoffload PRIVATE qoffload::core)
