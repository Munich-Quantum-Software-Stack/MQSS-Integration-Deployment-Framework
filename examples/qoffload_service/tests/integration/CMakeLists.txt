# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

find_package(GTest REQUIRED)

add_executable(qoffload-integration-test-qrm TestQrm.cpp)
target_link_libraries(qoffload-integration-test-qrm PRIVATE mqss::mqss)

target_include_directories(qoffload-integration-test-qrm
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

foreach(roundtrip IN ITEMS Direct Control)
  string(TOLOWER "${roundtrip}" roundtrip_name)
  set(target "qoffload-integration-${roundtrip_name}-roundtrip")

  add_executable(${target} "${roundtrip}Roundtrip.cpp")
  target_link_libraries(${target} PRIVATE mqss::mqss GTest::gtest_main)
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  add_test(
    NAME "Integration.${roundtrip}Roundtrip"
    COMMAND
      ${CMAKE_CURRENT_SOURCE_DIR}/run_roundtrip.sh "${roundtrip_name}"
      $<TARGET_FILE:qoffload> $<TARGET_FILE:qoffload-integration-test-qrm>
      $<TARGET_FILE:${target}>)
  set_tests_properties(
    "Integration.${roundtrip}Roundtrip" PROPERTIES LABELS "integration" TIMEOUT
                                                   45 RUN_SERIAL TRUE)
endforeach()
