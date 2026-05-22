# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

find_package(GTest REQUIRED)

# Add a GTest-based test executable and register it with CTest.
#
# Options:
#   MPI          Run the test through mpiexec.
#   CUSTOM_MAIN  Do not link GTest::gtest_main.
#
# One-value arguments:
#   RANKS        Number of MPI ranks (default: 2).
#
# Multi-value arguments:
#   LIBS         Additional libraries linked to the test target.
function(mqss_add_test name)
  set(options MPI CUSTOM_MAIN)
  set(oneValueArgs RANKS)
  set(multiValueArgs LIBS)

  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "mqss_add_test: Unrecognized arguments: "
      "${ARG_UNPARSED_ARGUMENTS}. "
      "Is the LIBS keyword missing?"
    )
  endif()

  add_executable(${name}
    ${name}.cpp
  )

  if(ARG_CUSTOM_MAIN)
    set(gtest_target GTest::gtest)
  else()
    set(gtest_target GTest::gtest_main)
  endif()

  target_link_libraries(${name}
    PRIVATE
      ${ARG_LIBS}
      ${gtest_target}
  )

  if(ARG_MPI)
    if(NOT ARG_RANKS)
      set(ARG_RANKS 2)
    endif()

    add_test(
      NAME ${name}
      COMMAND
        "${MPIEXEC_EXECUTABLE}"
        "${MPIEXEC_NUMPROC_FLAG}" "${ARG_RANKS}"
        "$<TARGET_FILE:${name}>"
    )
  else()
    add_test(NAME ${name} COMMAND ${name})
  endif()
endfunction()


mqss_add_test(ProtobufSerialization
  LIBS
    mqss_protocol
)

mqss_add_test(InMemoryTransportContract
  LIBS
    mqss_transport_inmemory
)

mqss_add_test(InMemoryTyped
  LIBS
    mqss_protocol
    mqss_transport_inmemory
)

if(MQSS_ENABLE_RABBITMQ_TRANSPORT)
  mqss_add_test(RabbitMqTransportContract
    LIBS
      mqss_transport_rabbitmq_simple
  )

  mqss_add_test(RabbitMqTyped
    LIBS
      mqss_protocol
      mqss_transport_rabbitmq_simple
  )
endif()

if(MQSS_ENABLE_MPI_TRANSPORT)
  mqss_add_test(MpiTransportContract
    MPI
    CUSTOM_MAIN
    RANKS 2
    LIBS
      mqss_transport_mpi
      MPI::MPI_CXX
)

  mqss_add_test(MpiTyped
    MPI
    CUSTOM_MAIN
    RANKS 2
    LIBS
      mqss_protocol
      mqss_transport_mpi
      MPI::MPI_CXX
  )
endif()
