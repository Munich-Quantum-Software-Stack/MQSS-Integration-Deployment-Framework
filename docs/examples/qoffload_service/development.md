# Development

## Prerequisites

Building QOffload requires CMake and the MQSS integration and configuration libraries.

Integration tests require access to RabbitMQ.
The repository's Docker development environment or an equivalent RabbitMQ installation can be used.

## Build

This application is built as an external consumer of an installed MQSS package.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/mqss-install
cmake --build build
```

## Tests

QOffload uses GTest and includes unit and integration tests.

Run all tests:

```sh
ctest --test-dir build
```

### Integration Tests

The integration tests exercise the QOffload binary in a round-trip scenario.
A launcher script starts the QOffload binary and a minimal QRM implementation before executing the selected test.
The test initiator sends requests to QOffload, which forwards them to the QRM implementation.
The corresponding responses are returned through QOffload to the test initiator.

The request-response path is:

`test initiator → QOffload → QRM → QOffload → test initiator`

When running outside the development container, connection settings may be provided through environment variables.

Run all integration tests:

```sh
ctest --test-dir build -L integration
```

Direct round-trip test validates the Task interface and direct result mapping.

```sh
ctest --test-dir build -R DirectRoundtrip
```

or

```sh
./tests/integration/run_roundtrip.sh \
    direct \
    ./build/qoffload \
    ./build/tests/integration/qoffload-integration-test-qrm \
    ./build/tests/integration/qoffload-integration-direct-roundtrip
```

Control round-trip test validates control task creation, QRM result processing, status queries, and result retrieval.

```sh
ctest --test-dir build -R ControlRoundtrip
```

or

```sh
./tests/integration/run_roundtrip.sh \
    control \
    ./build/qoffload \
    ./build/tests/integration/qoffload-integration-test-qrm \
    ./build/tests/integration/qoffload-integration-control-roundtrip
```

## Automated Check

Perform an end-to-end build and execution check:

```bash
scripts/check_qoffload_service.sh
```

The script builds MQSS, installs it, builds the QOffload service, and runs all tests.

## Docker Environment

Start the RabbitMQ-backed workflow development environment:

```bash
scripts/docker_workflow.sh
```

Within the container, build QOffload and run the tests:

```bash
scripts/check_qoffload_service.sh
```
