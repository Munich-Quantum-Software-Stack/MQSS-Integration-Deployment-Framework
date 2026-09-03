# Development

## Native Setup (Linux)

Required tools:

- CMake 3.28 or newer
- C++23-compatible toolchain
- protoc (Protocol Buffers compiler)
- protoc-gen-doc (v1.5.1)
  <https://github.com/pseudomuto/protoc-gen-doc/releases>
- GoogleTest
- RabbitMQ C library development package

### Project Dependencies

Required for MQSS development, testing, and documentation generation:

```bash
sudo apt install build-essential cmake protobuf-compiler libprotobuf-dev \
  libgtest-dev librabbitmq-dev libboost-chrono-dev libboost-system-dev \
  doxygen graphviz
```

### Project Examples

Additional dependencies required by the project examples:

```bash
sudo apt install libspdlog-dev libfmt-dev
```

### MQSSCI Dependencies

Additional dependencies required by the MQSSCI toolchain integration:

```bash
sudo apt install curl wget ninja-build libbz2-dev libssl-dev libffi-dev libboost-system-dev libboost-graph-dev libz3-dev \
  libncurses-dev libreadline-dev libsqlite3-dev liblzma-dev libzstd-dev
```

`examples/qrm_workflow` resolves the following as external CMake dependencies, pulled in automatically via `FetchContent` (see `examples/qrm_workflow/cmake/`) rather than as apt packages:

| Dependency | Repository                                                                                                        | Pinned Ref | Notes                                                                                                                                                        |
| ---------- | ----------------------------------------------------------------------------------------------------------------- | ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| MQSSCI     | [MQSS-Quantum-Compilation-Suite](https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite) | `v2.1.0`   | Compiler passes and pipelines (`mqss-ci::mqss-ci`); declared in `cmake/FindMQSSCI.cmake`                                                                     |
| QDMI       | [QDMI](https://github.com/Munich-Quantum-Software-Stack/QDMI.git)                                                 | `v1.3.2`   | Quantum Device Management Interface (`qdmi::qdmi`); declared in `cmake/Findqdmi.cmake`                                                                       |
| QInfo      | [QInfo](https://github.com/Munich-Quantum-Software-Stack/QInfo.git)                                               | `develop`  | Device/circuit info utilities; declared in `cmake/Findqinfo.cmake`                                                                                           |
| CUDA-Q     | [cuda-quantum](https://github.com/NVIDIA/cuda-quantum.git)                                                        | `0.15.0`   | MLIR Quake/CC/QEC dialects consumed by MQSSCI; auto-fetched and built from source on first configure (`CUDAQ_AUTO_FETCH`, set by `FindMQSSCI.cmake`)         |
| Catalyst   | [catalyst](https://github.com/PennyLaneAI/catalyst.git)                                                           | `v0.15.0`  | MLIR Quantum/QRef/MBQC dialects consumed by MQSSCI; auto-fetched and built from source on first configure (`CATALYST_AUTO_FETCH`, set by `FindMQSSCI.cmake`) |

CUDA-Q and Catalyst are built from source against this project's LLVM/MLIR on first configure, which can take a long time; subsequent configures reuse the cached build under `build/_deps/`. Both require `LLVM_DIR`/`MLIR_DIR` to already be set (i.e. `find_package(MLIR CONFIG)` must run before `find_package(MQSSCI)`).

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release  # or Debug
cmake --build build
```

## Run Tests

```bash
ctest --test-dir build
```

## Install

```bash
cmake --install build --prefix /tmp/mqss-install
```

The installation provides:

- public MQSS headers
- generated protobuf headers
- MQSS libraries
- vendored SimpleAmqpClient runtime library
- CMake package files for `find_package(mqss)`

## Use Installed Package

A consuming CMake project can use the installed package with:

```cmake
find_package(mqss REQUIRED)

target_link_libraries(app
  PRIVATE
    mqss::mqss
)
```

Configure the consumer with:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/mqss-install
cmake --build build
```

## Build Installed Consumer Example

```bash
cmake -S examples/mqss_consumer -B examples/mqss_consumer/build \
  -DCMAKE_PREFIX_PATH=/tmp/mqss-install

cmake --build examples/mqss_consumer/build

./examples/mqss_consumer/build/mqss_consumer
```

## Full Build and Installation Check

The helper script below performs:

- configure/build
- unit tests
- installation
- external consumer example configure/build/run

```bash
scripts/check_install.sh
```

Optional environment overrides:

```bash
CLEAN=1 \
BUILD_TYPE=Debug \
BUILD_DIR=build-debug \
INSTALL_DIR=/tmp/mqss-debug-install \
RUN_RABBITMQ_TESTS=1 \
scripts/check_install.sh
```

## Build and Run QRM Workflow Example

Perform an end-to-end build and execution check:

```bash
scripts/check_qrm_workflow.sh
```

See [QRM Workflow Example](examples/qrm_workflow.md) for details.

## Build and Test QOffload Service

Perform an end-to-end build and execution check:

```bash
scripts/check_qoffload_service.sh
```

See [QOffload Service](examples/qoffload_service.md) for details.

## Docker Development Container

Build and enter the MQSS development container:

```bash
NO_CACHE=1 scripts/docker_dev.sh
```

## Docker Workflow Environment

Start the RabbitMQ-backed workflow development environment:

```bash
scripts/docker_workflow.sh
```

This command builds the development image if needed, starts RabbitMQ using Docker Compose, and opens a shell in the workflow container.

The workflow container is configured to connect to the RabbitMQ service as:

```bash
QRM_AMQP_HOST=rabbitmq
```

Stop the workflow environment:

```bash
DOWN=1 scripts/docker_workflow.sh
```

## Generate Protocol Documentation

```bash
scripts/gen_proto_docs.sh
```

## Generate Doxygen Documentation

```bash
doxygen Doxyfile
```
