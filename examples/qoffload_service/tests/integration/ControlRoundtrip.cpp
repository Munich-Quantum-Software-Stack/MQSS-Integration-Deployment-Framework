// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Verifies task submission, status polling, and result retrieval via control
// API.

#include "TestSupport.hpp"

#include "mqss/Protocol.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <thread>

namespace {

mqss::APIRequest baseRequest(const std::string &responseQueue) {
  mqss::APIRequest request;
  request.set_authorization("integration-test-token");
  request.set_response_queue(responseQueue);
  return request;
}

mqss::APIRequest submitRequest(const std::string &responseQueue) {
  auto request = baseRequest(responseQueue);
  auto *create = request.mutable_create_task();
  create->set_shots(8);
  create->add_circuit("control-roundtrip.qasm");
  create->set_circuit_format("qasm2");
  create->set_resource_name("TestQRM");
  create->set_no_modify(false);
  return request;
}

mqss::APIRequest statusRequest(std::int32_t uuid,
                               const std::string &responseQueue) {
  auto request = baseRequest(responseQueue);
  request.mutable_task_status()->set_uuid(uuid);
  return request;
}

mqss::APIRequest resultRequest(std::int32_t uuid,
                               const std::string &responseQueue) {
  auto request = baseRequest(responseQueue);
  request.mutable_task_result()->set_uuid(uuid);
  return request;
}

std::optional<mqss::APIResponse>
sendRequest(qoffload::test::Messenger &messenger,
            const std::string &controlQueue, const mqss::APIRequest &request,
            const std::string &responseQueue) {
  const auto sendStatus =
      messenger.send<mqss::APIRequest>({controlQueue}, request);
  if (!sendStatus.ok()) {
    return std::nullopt;
  }

  auto response = messenger.receive<mqss::APIResponse>(
      {responseQueue},
      mqss::ReceiveArgs{.timeout = qoffload::test::receiveTimeout,
                        .ack_mode = mqss::AckMode::Auto});
  if (!response.has_value()) {
    return std::nullopt;
  }
  return std::move(*response);
}

} // namespace

// Exercises the complete control-task lifecycle across the running processes.
TEST(Integration, ControlRoundtrip) {
  const auto controlQueue = qoffload::test::getEnvOr(
      "QOFFLOAD_CONTROL_REQUEST_QUEUE", "qoffload.test.control.requests");
  const auto responseQueue =
      qoffload::test::getEnvOr("QOFFLOAD_TEST_CONTROL_RESPONSE_QUEUE",
                               "qoffload.test.control.responses");

  qoffload::test::Messenger messenger{qoffload::test::transportOptions()};

  const auto submit = sendRequest(messenger, controlQueue,
                                  submitRequest(responseQueue), responseQueue);
  ASSERT_TRUE(submit.has_value());
  ASSERT_TRUE(submit->has_create_task());
  const auto uuid = submit->create_task().uuid();

  std::optional<mqss::APIResponse> status;
  for (int attempt = 0; attempt < 20; ++attempt) {
    status = sendRequest(messenger, controlQueue,
                         statusRequest(uuid, responseQueue), responseQueue);
    ASSERT_TRUE(status.has_value());
    ASSERT_FALSE(status->has_error())
        << (status->has_error() ? status->error().message() : "");
    ASSERT_TRUE(status->has_task_status());

    if (status->task_status().status() ==
        mqss::ApiTaskStatus::API_TASK_STATUS_COMPLETED) {
      break;
    }
    ASSERT_NE(status->task_status().status(),
              mqss::ApiTaskStatus::API_TASK_STATUS_CANCELLED);
    std::this_thread::sleep_for(std::chrono::milliseconds{250});
  }

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(status->has_task_status());
  ASSERT_EQ(status->task_status().status(),
            mqss::ApiTaskStatus::API_TASK_STATUS_COMPLETED);

  const auto result =
      sendRequest(messenger, controlQueue, resultRequest(uuid, responseQueue),
                  responseQueue);
  ASSERT_TRUE(result.has_value());
  ASSERT_FALSE(result->has_error())
      << (result->has_error() ? result->error().message() : "");
  ASSERT_TRUE(result->has_task_result());
  ASSERT_EQ(result->task_result().result_size(), 1);
  EXPECT_EQ(result->task_result().result(0).counts().at("0"), 8);
}
