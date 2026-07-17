// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Tests for the REST-like control protocol adapter.

#include "ControlProtocol.hpp"

#include <google/protobuf/struct.pb.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace {

using qoffload::protocol::CreateQuantumTaskCommand;
using qoffload::protocol::GetTaskResultCommand;
using qoffload::protocol::InvalidControlCommand;
using qoffload::protocol::RestControlProtocol;
using qoffload::protocol::TaskCreatedResult;

void setString(google::protobuf::Struct &data, const std::string &key,
               const std::string &value) {
  (*data.mutable_fields())[key].set_string_value(value);
}

void setNumber(google::protobuf::Struct &data, const std::string &key,
               double value) {
  (*data.mutable_fields())[key].set_number_value(value);
}

void setBool(google::protobuf::Struct &data, const std::string &key,
             bool value) {
  (*data.mutable_fields())[key].set_bool_value(value);
}

mqss::APIRequest validCreateRequest() {
  mqss::APIRequest request;
  request.set_method("POST");
  request.set_request("/job/");
  request.set_response_queue("client.responses");

  auto &data = *request.mutable_data();
  setNumber(data, "shots", 128);
  auto *circuits = (*data.mutable_fields())["circuit"].mutable_list_value();
  circuits->add_values()->set_string_value("first.qasm");
  circuits->add_values()->set_string_value("second.qasm");
  setString(data, "circuit_format", "qasm2");
  setString(data, "resource_name", "Q20");
  setBool(data, "no_modify", true);
  return request;
}

} // namespace

TEST(RestControlProtocol, EncodesCreatedTask) {
  const RestControlProtocol protocol;

  const auto response =
      protocol.encode(TaskCreatedResult{.task_id = 987}, "client.responses");

  EXPECT_EQ(response.destination_queue(), "client.responses");
  const auto field = response.response_body().fields().find("uuid");
  ASSERT_NE(field, response.response_body().fields().end());
  ASSERT_EQ(field->second.kind_case(), google::protobuf::Value::kStringValue);
  EXPECT_EQ(field->second.string_value(), "987");
}

TEST(RestControlProtocol, DecodesCreateTask) {
  const RestControlProtocol protocol;
  const auto decoded = protocol.decode(validCreateRequest());

  EXPECT_EQ(decoded.response_queue, "client.responses");
  const auto *command = std::get_if<CreateQuantumTaskCommand>(&decoded.command);
  ASSERT_NE(command, nullptr);
  EXPECT_EQ(command->shots, 128);
  EXPECT_EQ(command->circuit_files,
            (std::vector<std::string>{"first.qasm", "second.qasm"}));
  EXPECT_EQ(command->circuit_format, "qasm2");
  EXPECT_EQ(command->resource_name, "Q20");
  EXPECT_TRUE(command->no_modify);
}

TEST(RestControlProtocol, RejectsFractionalShotCount) {
  const RestControlProtocol protocol;
  auto request = validCreateRequest();
  setNumber(*request.mutable_data(), "shots", 1.5);

  const auto decoded = protocol.decode(request);

  const auto *command = std::get_if<InvalidControlCommand>(&decoded.command);
  ASSERT_NE(command, nullptr);
  EXPECT_EQ(command->message, "INVALID JOB REQUEST");
}

TEST(RestControlProtocol, DecodesTaskResultPath) {
  const RestControlProtocol protocol;
  mqss::APIRequest request;
  request.set_method("GET");
  request.set_request("///job/123/result///");

  const auto decoded = protocol.decode(request);

  const auto *command = std::get_if<GetTaskResultCommand>(&decoded.command);
  ASSERT_NE(command, nullptr);
  EXPECT_EQ(command->task_id, std::uint64_t{123});
}
