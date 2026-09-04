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
find_package(Threads REQUIRED)

include(Config.cmake)

configure_file(
  ConfigDefaults.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ConfigDefaults.hpp
  @ONLY
)

add_library(
  qoffload-core
  src/Control.cpp
  src/Forward.cpp
  src/QOffload.cpp
  src/TaskStore.cpp
  src/control/RestControlProtocol.cpp
  src/control/TypedControlProtocol.cpp
)
add_library(qoffload::core ALIAS qoffload-core)

target_include_directories(
  qoffload-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

target_link_libraries(
  qoffload-core
  PUBLIC
    mqss::mqss
    spdlog::spdlog
    Threads::Threads
)

target_compile_options(
  qoffload-core
  PUBLIC
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(qoffload src/main.cpp)
target_link_libraries(qoffload PRIVATE qoffload::core)

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
