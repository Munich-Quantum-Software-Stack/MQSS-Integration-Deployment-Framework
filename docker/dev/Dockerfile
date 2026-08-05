# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

FROM ubuntu:25.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    \
    # Project dependencies
    ca-certificates \
    build-essential \
    cmake \
    git \
    protobuf-compiler \
    libprotobuf-dev \
    libgtest-dev \
    librabbitmq-dev \
    libboost-chrono-dev \
    libboost-system-dev \
    libboost-graph-dev \
    doxygen \
    graphviz \
    # Project examples
    libspdlog-dev \
    libfmt-dev \
    \
    # MQSSCI dependencies
    curl \
    python3 \
    python3-pip \
    python3-venv \
    libz3-dev \
    wget \
    ninja-build \
    libncurses-dev \
    libreadline-dev \
    libsqlite3-dev \
    liblzma-dev \
    libzstd-dev \
    libbz2-dev \
    libssl-dev \
    libffi-dev \
  && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv \
    && /opt/venv/bin/pip install --no-cache-dir --upgrade pip \
    && /opt/venv/bin/pip install --no-cache-dir lit

ENV PATH="/opt/venv/bin:${PATH}"

WORKDIR /workspace

CMD ["/bin/bash"]
