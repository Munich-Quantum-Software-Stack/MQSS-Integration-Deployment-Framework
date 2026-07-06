// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Entry point for the QOffload service.
///
/// Creates the top-level QOffload component.

#include "qoffload/QOffload.hpp"

int main() {
  qoffload::QOffload qoffload{};
  qoffload.start();

  // Keep the process alive until an explicit termination signal is received.
  // qoffload.requestStop();
  return 0;
}
