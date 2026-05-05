# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

function(mqss_add_transport name)
  cmake_parse_arguments(ARG "" "" "SOURCES;PUBLIC_LIBS;PRIVATE_LIBS" ${ARGN})

  set(target mqss_transport_${name})

  add_library(${target}
    ${ARG_SOURCES}
  )

  target_link_libraries(${target}
    PUBLIC
      ${ARG_PUBLIC_LIBS}
    PRIVATE
      ${ARG_PRIVATE_LIBS}
  )

  target_include_directories(${target}
    PUBLIC
      ${PROJECT_SOURCE_DIR}/include
  )
endfunction()
