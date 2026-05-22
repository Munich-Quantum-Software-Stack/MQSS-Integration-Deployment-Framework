# Development

## Native Setup (Linux)

Required tools:

- CMake 3.28 or newer
- C++23-compatible toolchain
- protoc (Protocol Buffers compiler)
- protoc-gen-doc (v1.5.1)  
  <https://github.com/pseudomuto/protoc-gen-doc/releases>
- GoogleTest
- RabbitMQ C library development package (for RabbitMQ transport)
- MPI implementation with C++ support (for MPI transport)

### System Install (Ubuntu/Debian)

```bash
sudo apt install build-essential cmake protobuf-compiler libprotobuf-dev \
  libgtest-dev librabbitmq-dev libopenmpi-dev openmpi-bin
````

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

## Docker Development Container

Build and enter the MQSS development container:

```bash
NO_CACHE=1 scripts/docker_dev.sh
```

## Generate Protocol Documentation

```bash
scripts/gen_proto_docs.sh
```

## Generate Doxygen Documentation

```bash
doxygen Doxyfile
```
