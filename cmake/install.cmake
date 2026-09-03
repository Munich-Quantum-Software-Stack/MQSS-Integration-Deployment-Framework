# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(MQSS_INSTALL_TARGETS mqss mqss_protocol mqss_transport_inmemory)

if(MQSS_ENABLE_RABBITMQ_TRANSPORT)
  list(APPEND MQSS_INSTALL_TARGETS mqss_transport_rabbitmq_simple
       SimpleAmqpClient)
endif()

install(
  TARGETS ${MQSS_INSTALL_TARGETS}
  EXPORT mqssTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/mqss
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.pb.h")

install(
  EXPORT mqssTargets
  FILE mqssTargets.cmake
  NAMESPACE mqss::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mqss)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/mqssConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/mqssConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mqss)

# Temporary package version until project-wide versioning is introduced via
# project(... VERSION ...).
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/mqssConfigVersion.cmake
  VERSION 0.0.1
  COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mqssConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/mqssConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mqss)
