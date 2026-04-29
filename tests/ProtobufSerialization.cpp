// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Round-trip tests for protobuf messages.
// Verifies binary and JSON serialization/parse behavior for predefined protocol
// messages.

#include "v1/messages.pb.h"

#include <google/protobuf/util/json_util.h>
#include <gtest/gtest.h>

#include <initializer_list>
#include <string>

namespace proto = mqss::protocol::v1;

// Helpers

enum class ProtobufFormat { Binary, Json };

template <typename Msg>
void roundTrip(const Msg &input, Msg &output, ProtobufFormat fmt) {
  switch (fmt) {
  case ProtobufFormat::Binary: {
    std::string buffer;
    ASSERT_TRUE(input.SerializeToString(&buffer))
        << "Binary serialization failed";
    ASSERT_TRUE(output.ParseFromString(buffer)) << "Binary parsing failed";
    break;
  }

  case ProtobufFormat::Json: {
    std::string json;
    auto status = google::protobuf::util::MessageToJsonString(input, &json);
    ASSERT_TRUE(status.ok())
        << "JSON serialization failed: " << status.ToString();

    status = google::protobuf::util::JsonStringToMessage(json, &output);
    ASSERT_TRUE(status.ok()) << "JSON parsing failed: " << status.ToString();
    break;
  }
  }
}

void expectRepeatedString(
    const google::protobuf::RepeatedPtrField<std::string> &values,
    std::initializer_list<const char *> expected) {
  ASSERT_EQ(values.size(), static_cast<int>(expected.size()));

  int i = 0;
  for (const auto *exp : expected) {
    EXPECT_EQ(values.Get(i), exp);
    ++i;
  }
}

// Test

class ProtobufSerialization : public ::testing::TestWithParam<ProtobufFormat> {
protected:
  ProtobufFormat format() const { return GetParam(); }
};

// Message Generators and Validators

TEST_P(ProtobufSerialization, QuantumTask) {
  proto::QuantumTask input;
  input.set_task_id(119);
  input.set_n_qbits(5);
  input.set_n_shots(1024);
  input.add_circuit_files("bell.qasm");
  input.add_circuit_files("ghz.qasm");
  input.set_circuit_file_type("openqasm");
  input.set_result_destination("results.queue");
  input.set_preferred_qpu("qpu-preferred");
  input.set_scheduled_qpu("qpu-scheduled");
  input.set_priority(7);
  input.set_optimisation_level(2);
  input.set_no_modify(true);
  input.set_transpiler_flag(false);
  input.set_result_type(3);
  input.set_submit_time("2026-03-24 09:37:02.076146");
  input.add_circuits_qiskit()->set_string_value("opaque-circuit-repr");
  input.set_additional_information("test task");
  input.add_restricted_resource_names("qpu-a");
  input.add_restricted_resource_names("qpu-b");
  input.set_user_identity("alice");
  input.set_token("secret-token");
  input.set_via_hpc(true);

  proto::QuantumTask output;
  roundTrip(input, output, format());

  EXPECT_EQ(output.task_id(), 119);
  EXPECT_EQ(output.n_qbits(), 5);
  EXPECT_EQ(output.n_shots(), 1024);
  expectRepeatedString(output.circuit_files(), {"bell.qasm", "ghz.qasm"});
  EXPECT_EQ(output.circuit_file_type(), "openqasm");
  EXPECT_EQ(output.result_destination(), "results.queue");
  EXPECT_EQ(output.preferred_qpu(), "qpu-preferred");
  EXPECT_EQ(output.scheduled_qpu(), "qpu-scheduled");
  EXPECT_EQ(output.priority(), 7);
  EXPECT_EQ(output.optimisation_level(), 2);
  EXPECT_TRUE(output.no_modify());
  EXPECT_FALSE(output.transpiler_flag());
  EXPECT_EQ(output.result_type(), 3);
  EXPECT_EQ(output.submit_time(), "2026-03-24 09:37:02.076146");

  ASSERT_EQ(output.circuits_qiskit_size(), 1);
  EXPECT_EQ(output.circuits_qiskit(0).kind_case(),
            google::protobuf::Value::kStringValue);
  EXPECT_EQ(output.circuits_qiskit(0).string_value(), "opaque-circuit-repr");

  EXPECT_EQ(output.additional_information(), "test task");
  expectRepeatedString(output.restricted_resource_names(), {"qpu-a", "qpu-b"});
  EXPECT_EQ(output.user_identity(), "alice");
  EXPECT_EQ(output.token(), "secret-token");
  EXPECT_TRUE(output.via_hpc());
}

TEST_P(ProtobufSerialization, QuantumResult) {
  proto::QuantumResult input;
  input.set_task_id(119);
  input.set_destination("results.queue");
  input.set_execution_status(true);
  input.set_executed_qpu("qpu-scheduled");
  input.add_executed_circuits("OPENQASM 2.0; // circuit 1");
  input.add_executed_circuits("OPENQASM 2.0; // circuit 2");
  input.set_additional_information("execution complete");
  input.set_execution_time(1.25);

  auto *res = input.add_results();
  (*res->mutable_counts())["00"] = 500;
  (*res->mutable_counts())["11"] = 524;

  proto::QuantumResult output;
  roundTrip(input, output, format());

  EXPECT_EQ(output.task_id(), 119);
  ASSERT_EQ(output.results_size(), 1);
  ASSERT_EQ(output.results(0).counts().size(), 2u);
  EXPECT_EQ(output.results(0).counts().at("00"), 500);
  EXPECT_EQ(output.results(0).counts().at("11"), 524);
  EXPECT_EQ(output.destination(), "results.queue");
  EXPECT_TRUE(output.execution_status());
  EXPECT_EQ(output.executed_qpu(), "qpu-scheduled");
  expectRepeatedString(
      output.executed_circuits(),
      {"OPENQASM 2.0; // circuit 1", "OPENQASM 2.0; // circuit 2"});
  EXPECT_EQ(output.additional_information(), "execution complete");
  EXPECT_DOUBLE_EQ(output.execution_time(), 1.25);
}

TEST_P(ProtobufSerialization, QSRegisterEntry) {
  proto::QSRegisterEntry input;
  input.set_qpu_name("qpu-a");
  input.set_queue_name("task.queue");
  input.set_control_queue_name("control.queue");
  input.set_n_qbits(27);
  input.set_qpu_type(1);
  input.set_deregister(false);

  proto::QSRegisterEntry output;
  roundTrip(input, output, format());

  EXPECT_EQ(output.qpu_name(), "qpu-a");
  EXPECT_EQ(output.queue_name(), "task.queue");
  EXPECT_EQ(output.control_queue_name(), "control.queue");
  EXPECT_EQ(output.n_qbits(), 27);
  EXPECT_EQ(output.qpu_type(), 1);
  EXPECT_FALSE(output.deregister());
}

TEST_P(ProtobufSerialization, QSRegistrationInfo) {
  proto::QSRegistrationInfo input;
  input.set_uid(101);
  input.set_heartbeat_queue("heartbeat.queue");
  input.set_heartbeat_interval(30);
  input.set_deregistered(false);

  proto::QSRegistrationInfo output;
  roundTrip(input, output, format());

  EXPECT_EQ(output.uid(), 101);
  EXPECT_EQ(output.heartbeat_queue(), "heartbeat.queue");
  EXPECT_EQ(output.heartbeat_interval(), 30);
  EXPECT_FALSE(output.deregistered());
}

TEST_P(ProtobufSerialization, QHeartBeat) {
  proto::QHeartBeat input;
  input.set_uid(101);
  input.set_qpu_name("qpu-a");
  input.set_queue_name("task.queue");
  input.set_status(proto::QS_STATUS_RUNNING);
  input.set_queue_length(3);

  proto::QHeartBeat output;
  roundTrip(input, output, format());

  EXPECT_EQ(output.uid(), 101);
  EXPECT_EQ(output.qpu_name(), "qpu-a");
  EXPECT_EQ(output.queue_name(), "task.queue");
  EXPECT_EQ(output.status(), proto::QS_STATUS_RUNNING);
  EXPECT_EQ(output.queue_length(), 3);
}

TEST_P(ProtobufSerialization, QResourceInfo) {
  proto::QResourceInfo input;
  input.set_name("qpu-a");
  input.set_num_qubits(27);
  input.set_connectivity("all-to-all");
  input.set_instructions("x,h,cx,measure");

  proto::QResourceInfo output;
  roundTrip(input, output, format());

  EXPECT_EQ(output.name(), "qpu-a");
  EXPECT_EQ(output.num_qubits(), 27);
  EXPECT_EQ(output.connectivity(), "all-to-all");
  EXPECT_EQ(output.instructions(), "x,h,cx,measure");
}

// Parameters

INSTANTIATE_TEST_SUITE_P(
    v1, ProtobufSerialization,
    ::testing::Values(ProtobufFormat::Binary, ProtobufFormat::Json),
    [](const ::testing::TestParamInfo<ProtobufFormat> &info) {
      return info.param == ProtobufFormat::Binary ? "Binary" : "Json";
    });
