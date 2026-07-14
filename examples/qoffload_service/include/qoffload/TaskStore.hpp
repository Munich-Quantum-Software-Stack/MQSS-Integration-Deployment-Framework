// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Thread-safe storage for pending tasks and completed control task results.

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace qoffload {

/// Origin of a task tracked by QOffload.
enum class PendingTaskKind { DirectTask, ControlTask };

/// Public task status used by control status queries.
enum class TaskStatus { Waiting, Completed, Cancelled, NotFound };

/// Mapping information for a task currently waiting for a QRM result.
struct PendingTask {
  PendingTaskKind kind{};
  std::uint64_t forwarded_task_id{};
  std::uint64_t original_task_id{};
  std::string original_result_destination;
};

/// Stored result for a control-submitted task.
///
/// Results are kept in a semantic form until the control client fetches them.
/// Control-specific response formatting is performed by the Control module.
struct CompletedControlTask {
  using Counts = std::unordered_map<std::string, int>;

  std::uint64_t task_id{};
  bool execution_status{};
  std::vector<Counts> result_counts;
  std::string cancel_reason;
  std::string timestamp_scheduled;
  std::string timestamp_submitted;
  std::string timestamp_completed;
};

/// Owns QOffload task state shared by forwarding, result processing, and
/// control handling.
///
/// Tracks pending QRM task IDs, direct-client result destinations, and
/// completed control task results. This class is the synchronization boundary
/// for mutable task state.
///
/// Thread safety: all public member functions are safe for concurrent use.
class TaskStore {
public:
  /// Registers a direct client task under its forwarded QRM task ID.
  void addDirectTask(PendingTask task);

  /// Registers a control-submitted task as pending.
  void addControlTask(std::uint64_t task_id);

  /// Removes and returns a pending task mapping for a completed QRM result.
  std::optional<PendingTask> takePending(std::uint64_t task_id);

  /// Returns the current status of a task known to QOffload.
  TaskStatus status(std::uint64_t task_id) const;

  std::size_t pendingCount() const;

  /// Stores a completed control task until it is fetched by the control client.
  void storeCompletedControlTask(CompletedControlTask result);

  /// Removes and returns a completed control task result.
  std::optional<CompletedControlTask>
  takeCompletedControlTask(std::uint64_t task_id);

private:
  /// Protects all mutable task state.
  mutable std::mutex mutex_;

  /// Pending tasks indexed by their forwarded QRM task identifier.
  std::unordered_map<std::uint64_t, PendingTask> pending_;

  /// Completed Control results retained until requested by a client.
  std::unordered_map<std::uint64_t, CompletedControlTask>
      completed_control_tasks_;
};

} // namespace qoffload
