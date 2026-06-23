# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

cmake_minimum_required(VERSION 3.28)
project(qrm_workflow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(mqss REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(MQSSCI REQUIRED)

include(Config.cmake)

configure_file(
  ConfigDefaults.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ConfigDefaults.hpp
  @ONLY
)

file(MAKE_DIRECTORY "${QRM_LOG_DIR}")

function(add_qrm_workflow_executable target)
  add_executable(${target}
    ${ARGN}
    Config.cpp
  )

  target_include_directories(${target}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/generated
  )

  target_link_libraries(${target}
    PRIVATE
      mqss::mqss
      spdlog::spdlog
  )
endfunction()

add_qrm_workflow_executable(daemon Daemon.cpp)
add_qrm_workflow_executable(scheduler Scheduler.cpp)
add_qrm_workflow_executable(compiler Compiler.cpp)
