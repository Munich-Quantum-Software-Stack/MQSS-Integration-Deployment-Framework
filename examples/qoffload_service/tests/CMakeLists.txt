# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

find_package(GTest REQUIRED)

add_executable(
  qoffload-unit-tests
  Sanity.cpp
)

target_include_directories(
  qoffload-unit-tests
  PRIVATE ${PROJECT_SOURCE_DIR}/src/control
)

target_link_libraries(
    qoffload-unit-tests
    PRIVATE
        qoffload::core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(qoffload-unit-tests)
