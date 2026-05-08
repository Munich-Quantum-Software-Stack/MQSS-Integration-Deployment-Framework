#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

set -eu

IMAGE_NAME="${IMAGE_NAME:-mqss-dev}"
DOCKERFILE="${DOCKERFILE:-docker/dev/Dockerfile}"
NO_CACHE="${NO_CACHE:-0}"

BUILD_ARGS=()

if [[ "${NO_CACHE}" == "1" ]]; then
  BUILD_ARGS+=(--no-cache)
fi

docker build \
  "${BUILD_ARGS[@]}" \
  -t "${IMAGE_NAME}" \
  -f "${DOCKERFILE}" \
  .

docker run --rm -it \
  -v "$(pwd):/workspace" \
  -w /workspace \
  "${IMAGE_NAME}"
