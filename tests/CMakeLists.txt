# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

find_package(GTest REQUIRED)

add_executable(ProtobufSerialization
  ProtobufSerialization.cpp
)

target_link_libraries(ProtobufSerialization
  PRIVATE
    mqss_protocol
    GTest::gtest_main
)

add_test(NAME ProtobufSerialization COMMAND ProtobufSerialization)
