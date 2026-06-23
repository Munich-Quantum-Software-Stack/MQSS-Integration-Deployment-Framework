#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

set -eu

cd "$(dirname "$0")/build"

./scheduler &
SCHEDULER_PID=$!

./compiler &
COMPILER_PID=$!

./daemon &
DAEMON_PID=$!

echo "All services started."
echo "  Scheduler PID: ${SCHEDULER_PID}"
echo "  Compiler  PID: ${COMPILER_PID}"
echo "  Daemon    PID: ${DAEMON_PID}"
echo "Press Ctrl+C to stop all"

cleanup() {
  echo
  echo "Stopping services..."
  kill -SIGTERM "${SCHEDULER_PID}" "${COMPILER_PID}" "${DAEMON_PID}" 2>/dev/null || true
  wait "${SCHEDULER_PID}" "${COMPILER_PID}" "${DAEMON_PID}" 2>/dev/null || true
  echo "Stopped."
}

trap cleanup SIGINT SIGTERM EXIT

wait
