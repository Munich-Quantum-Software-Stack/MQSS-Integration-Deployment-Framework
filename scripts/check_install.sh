#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
INSTALL_DIR="${INSTALL_DIR:-/tmp/mqss-install}"
EXAMPLE_BUILD_DIR="${EXAMPLE_BUILD_DIR:-examples/mqss_consumer/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if [[ "${CLEAN:-0}" == "1" ]]; then
  rm -rf "${BUILD_DIR}"
fi

cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

cmake --build "${BUILD_DIR}"

ctest --test-dir "${BUILD_DIR}" --output-on-failure

rm -rf "${INSTALL_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"

rm -rf "${EXAMPLE_BUILD_DIR}"
cmake -S examples/mqss_consumer -B "${EXAMPLE_BUILD_DIR}" \
  -DCMAKE_PREFIX_PATH="${INSTALL_DIR}"

cmake --build "${EXAMPLE_BUILD_DIR}"

"./${EXAMPLE_BUILD_DIR}/mqss_consumer"
