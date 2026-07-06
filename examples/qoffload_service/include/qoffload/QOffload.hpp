// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Top-level QOffload runtime that owns services, MQSS workers, and lifecycle.

#include "qoffload/Control.hpp"
#include "qoffload/Forward.hpp"

namespace qoffload {

/// Coordinates QOffload startup, worker threads, and shutdown.
///
/// QOffload owns all long-lived runtime objects. Message-processing logic
/// remains delegated to Forward and Control.
class QOffload {
public:
  QOffload() = default;
  ~QOffload() = default;

  QOffload(const QOffload &) = delete;
  QOffload &operator=(const QOffload &) = delete;
  QOffload(QOffload &&) = delete;
  QOffload &operator=(QOffload &&) = delete;

  /// Starts the QOffload workers if they are not already running.
  void start() {}

  /// Requests all workers to stop.
  void requestStop() {}
};

} // namespace qoffload
