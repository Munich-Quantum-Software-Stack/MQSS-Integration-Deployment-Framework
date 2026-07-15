// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Sanity tests for representative QOffload workflows.

#include "qoffload/Config.hpp"
#include "qoffload/Control.hpp"
#include "qoffload/Forward.hpp"
#include "qoffload/ResourceRegistry.hpp"
#include "qoffload/TaskStore.hpp"

#include <gtest/gtest.h>

#include <utility>

namespace {

qoffload::Config defaultConfig() {
  return qoffload::makeDefaultConfig(qoffload::ConfigTag{});
}

} // namespace

/// Verifies direct-task forwarding and restoration of client routing data.
TEST(Sanity, HandlesDirectTask) {
  qoffload::Forward forwarder{11};
  qoffload::TaskStore tasks;
  auto config = defaultConfig();
  config.queues.result = "qoffload.results";
  config.policy.hpc_node = true;

  mqss::QuantumTask request;
  request.set_task_id(1234);
  request.set_n_qbits(5);
  request.set_n_shots(1024);
  request.set_result_destination("client.results");
  request.set_priority(7);

  const auto forwarded = forwarder.forwardDirectTask(request, tasks, config);

  EXPECT_EQ(forwarded.task_id(), 11);
  EXPECT_EQ(forwarded.n_qbits(), 5);
  EXPECT_EQ(forwarded.n_shots(), 1024);
  EXPECT_EQ(forwarded.result_destination(), "qoffload.results");
  EXPECT_EQ(forwarded.priority(), 1);

  mqss::QuantumResult result;
  result.set_task_id(forwarded.task_id());
  result.set_destination("qoffload.results");
  result.set_execution_status(true);
  (*result.add_results()->mutable_counts())["0"] = 8;

  const auto outcome = forwarder.processResult(std::move(result), tasks);

  EXPECT_EQ(outcome.kind, qoffload::ResultProcessingKind::DirectResult);
  ASSERT_TRUE(outcome.direct_result.has_value());
  EXPECT_EQ(outcome.direct_result->task_id(), 1234);
  EXPECT_EQ(outcome.direct_result->destination(), "client.results");
  EXPECT_EQ(outcome.direct_result_destination, "client.results");
  EXPECT_EQ(tasks.status(11), qoffload::TaskStatus::NotFound);
}

/// Verifies creation, completion, status reporting, and result consumption of a
/// control-task.
TEST(Sanity, HandlesControlTask) {
  qoffload::Control control;
  qoffload::Forward forwarder{13};
  qoffload::TaskStore tasks;
  qoffload::ResourceRegistry resources;
  auto config = defaultConfig();
  config.queues.result = "qoffload.results";

  mqss::APIRequest create_request;
  create_request.set_response_queue("control.responses");
  auto *create = create_request.mutable_create_task();
  create->set_shots(1000);
  create->add_circuit("a.qasm");
  create->add_circuit("b.qasm");
  create->set_circuit_format("qasm2");
  create->set_resource_name("Q20");
  create->set_no_modify(true);

  const auto created =
      control.process(create_request, tasks, config, resources, forwarder);

  ASSERT_TRUE(created.outbound_task.has_value());
  EXPECT_EQ(created.outbound_task->task_id(), 13);
  EXPECT_EQ(created.outbound_task->result_destination(), "qoffload.results");
  EXPECT_EQ(created.outbound_task->n_shots(), 1000);
  EXPECT_EQ(created.outbound_task->circuit_file_type(), "qasm2");
  EXPECT_EQ(created.outbound_task->preferred_qpu(), "Q20");
  EXPECT_TRUE(created.outbound_task->no_modify());
  EXPECT_TRUE(created.outbound_task->via_hpc());
  ASSERT_EQ(created.outbound_task->circuit_files_size(), 2);
  ASSERT_TRUE(created.response.has_create_task());
  EXPECT_EQ(created.response.create_task().uuid(), 13);

  mqss::APIRequest status_request;
  status_request.set_response_queue("control.responses");
  status_request.mutable_task_status()->set_uuid(13);

  const auto waiting =
      control.process(status_request, tasks, config, resources, forwarder);
  ASSERT_TRUE(waiting.response.has_task_status());
  EXPECT_EQ(waiting.response.task_status().status(),
            mqss::ApiTaskStatus::API_TASK_STATUS_WAITING);

  mqss::QuantumResult result;
  result.set_task_id(created.outbound_task->task_id());
  result.set_execution_status(true);
  (*result.add_results()->mutable_counts())["0"] = 8;

  const auto processed = forwarder.processResult(std::move(result), tasks);
  EXPECT_EQ(processed.kind,
            qoffload::ResultProcessingKind::ControlResultStored);

  const auto completed =
      control.process(status_request, tasks, config, resources, forwarder);
  ASSERT_TRUE(completed.response.has_task_status());
  EXPECT_EQ(completed.response.task_status().status(),
            mqss::ApiTaskStatus::API_TASK_STATUS_COMPLETED);

  mqss::APIRequest result_request;
  result_request.set_response_queue("control.responses");
  result_request.mutable_task_result()->set_uuid(13);

  const auto retrieved =
      control.process(result_request, tasks, config, resources, forwarder);

  ASSERT_TRUE(retrieved.response.has_task_result());
  const auto &task_result = retrieved.response.task_result();
  ASSERT_EQ(task_result.result_size(), 1);
  EXPECT_EQ(task_result.result(0).counts().at("0"), 8);
  EXPECT_TRUE(task_result.timestamp_scheduled().empty());
  EXPECT_TRUE(task_result.timestamp_submitted().empty());
  EXPECT_TRUE(task_result.timestamp_completed().empty());
  EXPECT_EQ(tasks.status(13), qoffload::TaskStatus::NotFound);
}

/// Verifies resource information and pending-task reporting.
TEST(Sanity, ReportsResourceAndPendingTasks) {
  qoffload::Control control;
  qoffload::Forward forwarder{0};
  qoffload::TaskStore tasks;
  qoffload::ResourceRegistry resources;
  const auto config = defaultConfig();
  tasks.addControlTask(77);

  mqss::APIRequest resource_request;
  resource_request.set_response_queue("control.responses");
  resource_request.mutable_resource_info()->set_resource_name("Q20");

  const auto resource =
      control.process(resource_request, tasks, config, resources, forwarder);
  ASSERT_TRUE(resource.response.has_resource_info());
  EXPECT_EQ(resource.response.resource_info().name(), "Q20");
  EXPECT_EQ(resource.response.resource_info().num_qubits(), 20);
  EXPECT_TRUE(resource.response.resource_info().online());

  mqss::APIRequest pending_request;
  pending_request.set_response_queue("control.responses");
  pending_request.mutable_pending_tasks()->set_resource_name("Q20");

  const auto pending =
      control.process(pending_request, tasks, config, resources, forwarder);
  ASSERT_TRUE(pending.response.has_pending_tasks());
  EXPECT_EQ(pending.response.pending_tasks().num_pending_jobs(), 1);
}

/// Verifies that results without a pending task mapping are rejected.
TEST(Sanity, ReportsUnknownResultTask) {
  qoffload::Forward forwarder{0};
  qoffload::TaskStore tasks;
  mqss::QuantumResult result;
  result.set_task_id(555);

  const auto outcome = forwarder.processResult(std::move(result), tasks);

  EXPECT_EQ(outcome.kind, qoffload::ResultProcessingKind::UnknownTask);
  EXPECT_FALSE(outcome.direct_result.has_value());
}
