# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

cmake_minimum_required(VERSION 3.28)
project(qrm_workflow LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(mqss REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Spdlog REQUIRED)   # system installed
find_package(mqt-core REQUIRED)
find_package(MQSSCI REQUIRED)
find_package(qdmi REQUIRED)    # system by mqt-core
find_package(qinfo REQUIRED)

include(Config.cmake)

configure_file(
  ConfigDefaults.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ConfigDefaults.hpp
  @ONLY
)

include_directories(
  BEFORE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  ${GTEST_INCLUDE_DIRS}
)

file(MAKE_DIRECTORY "${QRM_LOG_DIR}")

function(add_qrm_workflow_executable target)
  add_executable(${target}
    ${ARGN}
    src/Config.cpp
  )

  target_include_directories(${target}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/generated
  )
  if(target STREQUAL "submitter")
    target_compile_definitions(
    ${target}
    PRIVATE
      "DYN_DEV_LIBS=std::pair{\"$<TARGET_FILE_NAME:MQT::CoreQDMINaDevice>\", \"MQT_NA\"}, std::pair{\"$<TARGET_FILE_NAME:MQT::CoreQDMIScDevice>\", \"MQT_SC\"}, std::pair{\"$<TARGET_FILE_NAME:MQT::CoreQDMI_DDSIM_Device>\", \"MQT_DDSIM\"}"
  )
  endif()

  target_link_libraries(${target}
    PRIVATE
      mqss::mqss
      spdlog::spdlog
      qdmi::qdmi
      MQT::CoreQDMIDriver
      qinfo
  )
endfunction()

add_qrm_workflow_executable(daemon src/Daemon.cpp)
add_qrm_workflow_executable(scheduler src/Scheduler.cpp)
add_qrm_workflow_executable(compiler src/Compiler.cpp)
add_qrm_workflow_executable(submitter src/Submitter.cpp)
