# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

cmake_minimum_required(VERSION 3.28)
project(mqss_consumer LANGUAGES CXX)

find_package(mqss REQUIRED)

add_executable(mqss_consumer
  main.cpp
)

target_link_libraries(mqss_consumer
  PRIVATE
    mqss::mqss
)
