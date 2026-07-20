#!/usr/bin/env bash

# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

# Build and install MQSS, then build and test the QOffload service example.

set -eu

BUILD_DIR="${BUILD_DIR:-build}"
INSTALL_DIR="${INSTALL_DIR:-/tmp/mqss-install}"
QOFFLOAD_DIR="${QOFFLOAD_DIR:-examples/qoffload_service}"
QOFFLOAD_BUILD_DIR="${QOFFLOAD_BUILD_DIR:-${QOFFLOAD_DIR}/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if [[ "${NO_CLEAN:-0}" == "0" ]]; then
  rm -rf "${BUILD_DIR}" "${QOFFLOAD_BUILD_DIR}"
fi

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

cmake --build "${BUILD_DIR}"

rm -rf "${INSTALL_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"

cmake -S "${QOFFLOAD_DIR}" -B "${QOFFLOAD_BUILD_DIR}" \
  -DCMAKE_PREFIX_PATH="${INSTALL_DIR}"

cmake --build "${QOFFLOAD_BUILD_DIR}"

if [[ "${NO_TEST:-0}" == "0" ]]; then
  ctest --test-dir "${QOFFLOAD_BUILD_DIR}" --output-on-failure
fi
