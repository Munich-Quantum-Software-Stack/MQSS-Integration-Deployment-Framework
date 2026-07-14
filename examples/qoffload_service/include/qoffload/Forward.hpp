// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Forwards tasks and correlates results across the execution pipeline.

#include "mqss/Protocol.hpp"

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

namespace qoffload {

struct Config;
class TaskStore;

/// Describes how an incoming execution result was handled.
enum class ResultProcessingKind {
  UnknownTask,
  DirectResult,
  ControlResultStored,
};

/// Result of correlating one execution result with pending task state.
struct ResultProcessingOutcome {
  /// Action taken for the received result.
  ResultProcessingKind kind{ResultProcessingKind::UnknownTask};

  /// Result remapped for direct client delivery, when applicable.
  std::optional<mqss::QuantumResult> direct_result;

  /// Destination queue for a remapped direct result.
  std::string direct_result_destination;
};

/// Moves tasks and results through QOffload's execution pipeline.
///
/// Forward assigns component-local task IDs, records task mappings, prepares
/// outbound tasks, and correlates returned results with their originating
/// direct or Control-interface submissions.
class Forward {
public:
  /// Creates the forwarding service using the QOffload instance UID.
  explicit Forward(std::uint64_t instance_uid) noexcept
      : instance_uid_(instance_uid) {}

  /// Rewrites a direct client task for the next execution stage.
  mqss::QuantumTask forwardDirectTask(const mqss::QuantumTask &request,
                                      TaskStore &tasks, const Config &config);

  /// Creates and registers an outbound task for a Control submission.
  mqss::QuantumTask createControlTask(mqss::QuantumTask task, TaskStore &tasks,
                                      const Config &config);

  /// Correlates one returned result with pending task state.
  ResultProcessingOutcome processResult(mqss::QuantumResult result,
                                        TaskStore &tasks) const;

private:
  /// Returns the next component-local task identifier.
  std::uint64_t nextTaskId();

  /// Numeric identifier of this QOffload instance.
  std::uint64_t instance_uid_{};

  /// Monotonic index used to generate unique forwarded task identifiers.
  std::atomic<std::uint64_t> next_task_index_{};
};

} // namespace qoffload
