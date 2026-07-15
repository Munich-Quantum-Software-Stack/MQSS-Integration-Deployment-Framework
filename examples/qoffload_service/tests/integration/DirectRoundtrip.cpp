// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Verifies a direct task roundtrip through QOffload and a QRM process.

#include "TestSupport.hpp"

#include "mqss/Protocol.hpp"

#include <gtest/gtest.h>

namespace {

mqss::QuantumTask makeTask(const std::string &resultQueue) {
  mqss::QuantumTask task;
  task.set_task_id(42);
  task.set_result_destination(resultQueue);
  task.set_n_qbits(1);
  task.set_n_shots(8);
  task.set_circuit_file_type("qasm2");
  task.set_preferred_qpu("TestQRM");
  task.add_circuit_files("direct-roundtrip.qasm");
  return task;
}

} // namespace

// Ensures QOffload restores the client task identifier on the returned result.
TEST(Integration, DirectRoundtrip) {
  const auto requestQueue = qoffload::test::getEnvOr(
      "QOFFLOAD_REQUEST_QUEUE", "qoffload.test.direct.requests");
  const auto resultQueue =
      qoffload::test::getEnvOr("QOFFLOAD_TEST_CLIENT_RESULT_QUEUE",
                               "qoffload.test.direct.client.results");

  qoffload::test::Messenger messenger{qoffload::test::transportOptions()};
  const auto task = makeTask(resultQueue);

  const auto sendStatus =
      messenger.send<mqss::QuantumTask>({requestQueue}, task);
  ASSERT_TRUE(sendStatus.ok()) << sendStatus.reason();

  auto result = messenger.receive<mqss::QuantumResult>(
      {resultQueue},
      mqss::ReceiveArgs{.timeout = qoffload::test::receiveTimeout,
                        .ack_mode = mqss::AckMode::Auto});
  ASSERT_TRUE(result.has_value()) << result.error().reason();

  EXPECT_EQ(result->task_id(), task.task_id());
  EXPECT_TRUE(result->execution_status());
  EXPECT_EQ(result->executed_qpu(), "TestQRM");
  ASSERT_EQ(result->results_size(), 1);
  EXPECT_EQ(result->results(0).counts().at("0"), task.n_shots());
}
