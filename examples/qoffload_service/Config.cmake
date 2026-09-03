# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception Copyright (c) MQSS
# Maintainers

# Dynamic defaults generated from the build-system context.

set(QOFFLOAD_RUNTIME_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/runtime"
    CACHE PATH "Default QOffload runtime directory")

set(QOFFLOAD_LOG_DIR
    "${QOFFLOAD_RUNTIME_DIR}/logs"
    CACHE PATH "Default QOffload log directory")

set(QOFFLOAD_STATE_FILE
    "${QOFFLOAD_RUNTIME_DIR}/qoffload-state.bin"
    CACHE FILEPATH "Default QOffload state file")
