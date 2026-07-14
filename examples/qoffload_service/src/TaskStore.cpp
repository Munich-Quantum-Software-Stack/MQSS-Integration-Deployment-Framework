// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "qoffload/TaskStore.hpp"

namespace qoffload {

void TaskStore::addDirectTask(PendingTask task) {
  std::scoped_lock lock{mutex_};
  pending_.insert_or_assign(task.forwarded_task_id, std::move(task));
}

void TaskStore::addControlTask(std::uint64_t task_id) {
  addDirectTask(PendingTask{.kind = PendingTaskKind::ControlTask,
                            .forwarded_task_id = task_id,
                            .original_task_id = task_id,
                            .original_result_destination = {}});
}

std::optional<PendingTask> TaskStore::takePending(std::uint64_t task_id) {
  std::scoped_lock lock{mutex_};

  const auto it = pending_.find(task_id);
  if (it == pending_.end()) {
    return std::nullopt;
  }

  auto task = std::move(it->second);
  pending_.erase(it);
  return task;
}

TaskStatus TaskStore::status(std::uint64_t task_id) const {
  std::scoped_lock lock{mutex_};

  if (pending_.contains(task_id)) {
    return TaskStatus::Waiting;
  }

  const auto completed = completed_control_tasks_.find(task_id);
  if (completed == completed_control_tasks_.end()) {
    return TaskStatus::NotFound;
  }

  return completed->second.execution_status ? TaskStatus::Completed
                                            : TaskStatus::Cancelled;
}

std::size_t TaskStore::pendingCount() const {
  std::scoped_lock lock{mutex_};
  return pending_.size();
}

void TaskStore::storeCompletedControlTask(CompletedControlTask result) {
  std::scoped_lock lock{mutex_};
  completed_control_tasks_.insert_or_assign(result.task_id, std::move(result));
}

std::optional<CompletedControlTask>
TaskStore::takeCompletedControlTask(std::uint64_t task_id) {
  std::scoped_lock lock{mutex_};

  const auto it = completed_control_tasks_.find(task_id);
  if (it == completed_control_tasks_.end()) {
    return std::nullopt;
  }

  auto result = std::move(it->second);
  completed_control_tasks_.erase(it);
  return result;
}

} // namespace qoffload
