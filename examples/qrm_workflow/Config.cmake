# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

# Dynamic defaults generated via cmake build-system context

# Compiler toolchain
set(QRM_CUDAQ_QUAKE
    "${MQSSCI_SRC_DIR}/_deps/mqss-cudaq/cudaq/bin/cudaq-quake"
    CACHE PATH "cudaq-quake executable")

set(QRM_CUDAQ_TRANSLATE
    "${MQSSCI_SRC_DIR}/_deps/mqss-cudaq/cudaq/bin/cudaq-translate"
    CACHE PATH "cudaq-translate executable")

set(QRM_CUDAQ_OPT
    "${MQSSCI_SRC_DIR}/_deps/mqss-cudaq/cudaq/bin/cudaq-opt"
    CACHE PATH "cudaq-opt executable")

set(QRM_MQSS_CUDAQ_OPT
    "${MQSSCI_INSTALL_DIR}/mqss-cudaq-opt"
    CACHE PATH "mqss-cudaq-opt executable")

# Logging
set(QRM_LOG_DIR "${CMAKE_CURRENT_BINARY_DIR}/logs"
  CACHE PATH "Default log directory")

# Benchmarks
set(QRM_BENCHMARK_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    CACHE PATH "Benchmark directory")
