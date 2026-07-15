// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Minimal QRM process used by the integration roundtrips.

#include "TestSupport.hpp"

#include "mqss/Protocol.hpp"

#include <iostream>

namespace {

mqss::QuantumResult makeResult(const mqss::QuantumTask &task) {
  mqss::QuantumResult result;
  result.set_task_id(task.task_id());
  result.set_destination(task.result_destination());
  result.set_execution_status(true);
  result.set_executed_qpu(task.preferred_qpu().empty() ? "TestQRM"
                                                       : task.preferred_qpu());
  result.add_executed_circuits("OPENQASM 2.0; // integration test");
  result.set_additional_information("integration test QRM completed the task");
  result.set_execution_time(0.001);

  auto *counts = result.add_results()->mutable_counts();
  (*counts)["0"] = task.n_shots() > 0 ? task.n_shots() : 1;
  return result;
}

} // namespace

int main() {
  const auto taskQueue =
      qoffload::test::getEnvOr("QRM_TASK_QUEUE", "qoffload.test.qrm.tasks");

  qoffload::test::Messenger messenger{qoffload::test::transportOptions()};
  auto task = messenger.receive<mqss::QuantumTask>(
      {taskQueue}, mqss::ReceiveArgs{.timeout = qoffload::test::receiveTimeout,
                                     .ack_mode = mqss::AckMode::Auto});
  if (!task.has_value()) {
    std::cerr << "test QRM failed to receive task: " << task.error().reason()
              << '\n';
    return 1;
  }

  const auto status = messenger.send<mqss::QuantumResult>(
      {task->result_destination()}, makeResult(*task));
  if (!status.ok()) {
    std::cerr << "test QRM failed to send result: " << status.reason() << '\n';
    return 1;
  }

  return 0;
}
