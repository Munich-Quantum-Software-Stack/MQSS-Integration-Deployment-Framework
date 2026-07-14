// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Top-level QOffload runtime that owns services, MQSS workers, and lifecycle.

#include "qoffload/Config.hpp"
#include "qoffload/Control.hpp"
#include "qoffload/Forward.hpp"
#include "qoffload/ResourceRegistry.hpp"
#include "qoffload/TaskStore.hpp"

#include <spdlog/logger.h>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace qoffload {

/// Coordinates QOffload startup, worker threads, and shutdown.
///
/// QOffload owns all long-lived runtime objects. Message-processing logic
/// remains delegated to Forward, Control, and TaskStore.
///
/// Thread safety: start() and requestStop() are intended to be called from the
/// controlling thread. Worker state is stopped cooperatively through jthreads.
class QOffload {
public:
  QOffload(Config config, std::shared_ptr<spdlog::logger> logger);
  ~QOffload();

  QOffload(const QOffload &) = delete;
  QOffload &operator=(const QOffload &) = delete;
  QOffload(QOffload &&) = delete;
  QOffload &operator=(QOffload &&) = delete;

  /// Starts the QOffload workers if they are not already running.
  void start();

  /// Requests all workers to stop and lets jthread ownership join them.
  void requestStop();

  /// Returns whether the worker set is active.
  bool running() const noexcept { return running_.load(); }

private:
  /// Starts workers for task submission, Control requests, and QRM results.
  void startWorkers();

  /// Immutable runtime configuration owned by the service.
  Config config_;

  /// Logger shared by all worker threads.
  std::shared_ptr<spdlog::logger> logger_;

  /// Shared pending and completed task state.
  TaskStore tasks_;

  /// Resource information exposed through the Control interface.
  ResourceRegistry resources_;

  /// Task forwarding and result-correlation service.
  Forward forwarder_;

  /// Control request processor.
  Control control_;

  /// Long-lived message-processing workers.
  std::vector<std::jthread> workers_;

  /// Indicates whether the worker set is active.
  std::atomic_bool running_{false};
};

} // namespace qoffload
