#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

# Build, install, and run the QRM workflow example.

set -eu

BUILD_DIR="${BUILD_DIR:-build}"
INSTALL_DIR="${INSTALL_DIR:-/tmp/mqss-install}"
QRM_BUILD_DIR="${QRM_BUILD_DIR:-examples/qrm_workflow/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
RUN_WORKFLOW="${RUN_WORKFLOW:-1}"

if [[ "${CLEAN:-0}" == "1" ]]; then
  rm -rf "${BUILD_DIR}" "${QRM_BUILD_DIR}"
fi

cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

cmake --build "${BUILD_DIR}"

rm -rf "${INSTALL_DIR}"

cmake --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"

rm -rf "${QRM_BUILD_DIR}"     # Indiscriminately removing build is not a good idea since all dependencies go inside it (Keeping it for now)
                                
cmake -S examples/qrm_workflow -B "${QRM_BUILD_DIR}" \
  -DCMAKE_PREFIX_PATH="${INSTALL_DIR}"
  
cmake --build "${QRM_BUILD_DIR}"

if [[ "${RUN_WORKFLOW}" == "1" ]]; then
  (
    cd examples/qrm_workflow
    ./start.sh
  )
fi
