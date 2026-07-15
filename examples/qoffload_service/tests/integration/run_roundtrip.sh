#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

# Configures the test environment, starts QOffload and the test QRM as
# background processes, runs a client, and ensures both processes are cleaned
# up on exit.

set -eu

if [[ $# -ne 4 ]]; then
  echo "usage: $0 <name> <qoffload> <test-qrm> <test-client>" >&2
  exit 2
fi

name=$1
qoffload=$2
test_qrm=$3
test_client=$4

for executable in "$qoffload" "$test_qrm" "$test_client"; do
  if [[ ! -x "$executable" ]]; then
    echo "missing executable: $executable" >&2
    exit 1
  fi
done

prefix="qoffload.test.${name}.$$.${RANDOM}"
export QOFFLOAD_REQUEST_QUEUE="${prefix}.requests"
export QOFFLOAD_CONTROL_REQUEST_QUEUE="${prefix}.control.requests"
export QOFFLOAD_RESULT_QUEUE="${prefix}.qoffload.results"
export QRM_TASK_QUEUE="${prefix}.qrm.tasks"
export QOFFLOAD_TEST_CLIENT_RESULT_QUEUE="${prefix}.client.results"
export QOFFLOAD_TEST_CONTROL_RESPONSE_QUEUE="${prefix}.control.responses"
export QOFFLOAD_INSTANCE_UID=${QOFFLOAD_INSTANCE_UID:-7}

# Match development settings while preserving explicit overrides.
export QOFFLOAD_AMQP_HOST="${QOFFLOAD_AMQP_HOST:-127.0.0.1}"
export QOFFLOAD_AMQP_USER="${QOFFLOAD_AMQP_USER:-myuser}"
export QOFFLOAD_AMQP_PASSWORD="${QOFFLOAD_AMQP_PASSWORD:-mypassword}"
export QRM_AMQP_USER="${QRM_AMQP_USER:-$QOFFLOAD_AMQP_USER}"
export QRM_AMQP_PASSWORD="${QRM_AMQP_PASSWORD:-$QOFFLOAD_AMQP_PASSWORD}"

# Exit handler to clean up background processes.
cleanup() {
  if [[ -n "${qoffload_pid:-}" ]]; then
    kill "$qoffload_pid" 2>/dev/null || true
  fi
  if [[ -n "${qrm_pid:-}" ]]; then
    kill "$qrm_pid" 2>/dev/null || true
  fi
  wait 2>/dev/null || true
}
trap cleanup EXIT

# Start the QOffload executable under test.
"$qoffload" &
qoffload_pid=$!
sleep 1

# Start the test QRM.
"$test_qrm" &
qrm_pid=$!
sleep 1

# Run the integration test.
"$test_client"

# Wait for QRM to finish, then stop QOffload.
wait "$qrm_pid"
kill -TERM "$qoffload_pid" 2>/dev/null || true
wait "$qoffload_pid" 2>/dev/null || true

# Prevent cleanup() from killing already-exited processes.
qoffload_pid=
qrm_pid=
