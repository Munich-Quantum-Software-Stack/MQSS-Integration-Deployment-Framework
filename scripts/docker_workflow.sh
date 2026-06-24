#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

# Start/stop the RabbitMQ-backed workflow development environment.

set -eu

COMPOSE_FILE="${COMPOSE_FILE:-docker/compose/rabbitmq-workflow.yaml}"

if [[ "${DOWN:-0}" == "1" ]]; then
  docker compose -f "${COMPOSE_FILE}" down
  exit 0
fi

docker compose -f "${COMPOSE_FILE}" up -d --build

docker compose -f "${COMPOSE_FILE}" exec workflow bash
