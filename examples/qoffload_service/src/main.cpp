// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Entry point for the QOffload service.
///
/// Initializes the process configuration and creates the top-level QOffload
/// component.

#include "qoffload/Config.hpp"
#include "qoffload/QOffload.hpp"

#include <iostream>
#include <utility>

int main(int argc, char **argv) {
  auto config = qoffload::loadConfig(argc, argv);
  if (!config) {
    std::cerr << config.error().reason() << '\n';
    return 1;
  }

  if (auto status = qoffload::initConfig(std::move(*config)); !status.ok()) {
    std::cerr << status.reason() << '\n';
    return 1;
  }

  qoffload::QOffload qoffload{};
  qoffload.start();

  // Keep the process alive until an explicit termination signal is received.
  // qoffload.requestStop();
  return 0;
}
