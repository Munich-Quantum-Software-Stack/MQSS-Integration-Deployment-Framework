# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

FROM ubuntu:25.04

ENV DEBIAN_FRONTEND=noninteractive

# Allow OpenMPI execution inside the development container.
# The container currently runs as root.
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    protobuf-compiler \
    libprotobuf-dev \
    libgtest-dev \
    librabbitmq-dev \
    libboost-chrono-dev \
    libboost-system-dev \
    doxygen \
    graphviz \
    libopenmpi-dev \
    openmpi-bin \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
