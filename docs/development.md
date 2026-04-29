# Development

## Native Setup (Linux)

Required tools:

- cmake
- C++ toolchain
- protoc (Protocol Buffers compiler)
- protoc-gen-doc (v1.5.1)  
  <https://github.com/pseudomuto/protoc-gen-doc/releases>
- GoogleTest

### System Install (Ubuntu/Debian)

```bash
sudo apt install build-essential cmake protobuf-compiler libprotobuf-dev \ 
libgtest-dev
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release  # or Debug
cmake --build build
```

## Run Tests

```bash
ctest --test-dir build
```

## Generate Protocol Documentation

```bash
scripts/gen_proto_docs.sh
```
