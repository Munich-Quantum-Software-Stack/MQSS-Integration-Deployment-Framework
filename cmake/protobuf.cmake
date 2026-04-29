# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

find_package(Protobuf REQUIRED)

function(mqss_configure_proto_target target_name)
  set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol/proto/v1/messages.proto
  )

  set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
  set(CXX_PROTO_OUT_DIR ${GENERATED_DIR}/v1)

  file(MAKE_DIRECTORY ${CXX_PROTO_OUT_DIR})

  target_sources(${target_name}
    PRIVATE
      ${PROTO_FILES}
  )

  target_link_libraries(${target_name}
    PUBLIC
      protobuf::libprotobuf
  )

  protobuf_generate(
    TARGET ${target_name}
    LANGUAGE cpp
    PROTOC_OUT_DIR ${CXX_PROTO_OUT_DIR}
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/protocol/proto
    APPEND_PATH
  )

  target_include_directories(${target_name}
    PUBLIC
      ${GENERATED_DIR}
  )
endfunction()
