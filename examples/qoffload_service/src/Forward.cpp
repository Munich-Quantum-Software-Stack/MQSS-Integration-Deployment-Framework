// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "qoffload/Forward.hpp"

#include "qoffload/Config.hpp"
#include "qoffload/TaskStore.hpp"

#include <utility>

namespace qoffload {

mqss::QuantumTask Forward::forwardDirectTask(const mqss::QuantumTask &request,
                                             TaskStore &tasks,
                                             const Config &config) {
  const auto forwarded_task_id = nextTaskId();

  mqss::QuantumTask forwarded{request};
  forwarded.set_task_id(forwarded_task_id);
  forwarded.set_result_destination(config.queues.result);

  if (config.policy.hpc_node) {
    forwarded.set_priority(1);
  }

  tasks.addDirectTask(PendingTask{
      .kind = PendingTaskKind::DirectTask,
      .forwarded_task_id = forwarded_task_id,
      .original_task_id = static_cast<std::uint64_t>(request.task_id()),
      .original_result_destination = request.result_destination(),
  });

  return forwarded;
}

mqss::QuantumTask Forward::createControlTask(mqss::QuantumTask task,
                                             TaskStore &tasks,
                                             const Config &config) {
  const auto task_id = nextTaskId();

  task.set_task_id(task_id);
  task.set_result_destination(config.queues.result);
  if (config.policy.hpc_node) {
    task.set_priority(1);
  }

  tasks.addControlTask(task_id);
  return task;
}

ResultProcessingOutcome Forward::processResult(mqss::QuantumResult result,
                                               TaskStore &tasks) const {
  const auto forwarded_task_id = static_cast<std::uint64_t>(result.task_id());
  auto pending = tasks.takePending(forwarded_task_id);
  if (!pending.has_value()) {
    return {};
  }

  if (pending->kind == PendingTaskKind::ControlTask) {
    CompletedControlTask completed;
    completed.task_id = pending->original_task_id;
    completed.execution_status = result.execution_status();

    for (const auto &circuit_result : result.results()) {
      CompletedControlTask::Counts counts;
      for (const auto &entry : circuit_result.counts()) {
        counts.emplace(entry.first, static_cast<int>(entry.second));
      }
      completed.result_counts.push_back(std::move(counts));
    }

    tasks.storeCompletedControlTask(std::move(completed));
    return {
        .kind = ResultProcessingKind::ControlResultStored,
        .direct_result = std::nullopt,
        .direct_result_destination = {},
    };
  }

  result.set_task_id(pending->original_task_id);
  result.set_destination(pending->original_result_destination);

  return {
      .kind = ResultProcessingKind::DirectResult,
      .direct_result = std::move(result),
      .direct_result_destination = pending->original_result_destination,
  };
}

std::uint64_t Forward::nextTaskId() {
  const auto index = next_task_index_.fetch_add(1, std::memory_order_relaxed);
  return index * 10000 + instance_uid_;
}

} // namespace qoffload
