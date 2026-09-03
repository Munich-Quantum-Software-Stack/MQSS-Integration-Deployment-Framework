# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception Copyright (c) MQSS
# Maintainers

find_package(GTest REQUIRED)

function(mqss_add_test name)
  add_executable(${name} ${name}.cpp)

  target_link_libraries(${name} PRIVATE ${ARGN} GTest::gtest_main)

  add_test(NAME ${name} COMMAND ${name})
endfunction()

mqss_add_test(ProtobufSerialization mqss_protocol)

mqss_add_test(InMemoryTransportContract mqss_transport_inmemory)

mqss_add_test(InMemoryTyped mqss_protocol mqss_transport_inmemory)

if(MQSS_ENABLE_RABBITMQ_TRANSPORT)
  mqss_add_test(RabbitMqTransportContract mqss_transport_rabbitmq_simple)

  mqss_add_test(RabbitMqTyped mqss_protocol mqss_transport_rabbitmq_simple)
endif()
