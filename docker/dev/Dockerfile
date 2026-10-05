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
    \
    # Documentation dependencies
    doxygen \
    graphviz \
    python3 \
    python3-venv \
    \
    # Project examples
    libspdlog-dev \
    libfmt-dev \
    \
    # MQSSCI dependencies
    curl \
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

RUN python3 -m venv /opt/mkdocs \
 && /opt/mkdocs/bin/pip install --no-cache-dir \
      "mkdocs==1.6.1" \
      "mkdocs-material==9.7.7"

ENV PATH="/opt/mkdocs/bin:${PATH}"

WORKDIR /workspace

CMD ["/bin/bash"]
