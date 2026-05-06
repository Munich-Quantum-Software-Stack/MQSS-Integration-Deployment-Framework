// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Contract tests for the Transport API using the RabbitMQ transport.
//
// Verifies:
// - basic send/receive behavior
// - timeout semantics
// - unsupported features (send confirmation)
// - advertised acknowledgment-related features
// - manual acknowledgment behavior
//
// Notes:
// - Uses unique queue names to avoid interference between test runs.
// - Assumes a RabbitMQ broker is running on localhost:5672.
// - Each test run leaves one queue per test address on the broker.
//   These queues are declared as regular queues by the transport and are not
//   automatically deleted.

#include "mqss/Message.hpp"
#include "mqss/Status.hpp"
#include "mqss/transport/RabbitMqSimpleTransport.hpp"
#include "mqss/transport/Transport.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <random>
#include <string>

using namespace std::chrono_literals;

namespace {

// State for RabbitMQ availability (updated in the first test
// BrokerAvailability).
bool rabbitmq_up = false;
std::string rabbitmq_error;

#define REQUIRE_RABBITMQ()                                                     \
  if (!rabbitmq_up) {                                                          \
    GTEST_SKIP() << "RabbitMQ unavailable: " << rabbitmq_error;                \
  }

// Helper: check if Result<T> contains a specific Status code.
template <class T>
bool hasStatusCode(const mqss::Result<T> &result, mqss::StatusCode code) {
  return !result.has_value() && result.error().code() == code;
}

// Generate a unique queue name for one test case.
static std::string uniqueName(const std::string &prefix) {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  return prefix + "." + std::to_string(rng());
}

} // namespace

// Check if the broker is available, if not skip the following tests.
TEST(RabbitMqTransportContract, BrokerAvailability) {
  auto transport = mqss::createTransport<mqss::RabbitMqSimple>();
  mqss::Address address{uniqueName("queue.probe")};

  auto result = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 0ms, .ack_mode = mqss::AckMode::Auto});

  if (result.has_value()) {
    rabbitmq_up = true;
    return;
  }

  if (result.error().code() == mqss::StatusCode::Timeout) {
    rabbitmq_up = true;
    return;
  }

  rabbitmq_error = result.error().reason();

  FAIL() << "RabbitMQ unavailable: " << rabbitmq_error;
}

// send() -> receive() basic roundtrip.
TEST(RabbitMqTransportContract, SendThenReceive) {
  REQUIRE_RABBITMQ();

  auto transport = mqss::createTransport<mqss::RabbitMqSimple>();
  mqss::Address address{uniqueName("queue.send_receive")};

  mqss::Envelope envelope;
  envelope.payload = "hello";

  EXPECT_TRUE(transport->send(address, envelope).ok());

  auto result = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 1000ms, .ack_mode = mqss::AckMode::Auto});

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->envelope.payload, "hello");
}

// receive() with zero timeout acts as non-blocking poll.
TEST(RabbitMqTransportContract, ReceiveReturnsTimeoutForZeroTimeout) {
  REQUIRE_RABBITMQ();

  auto transport = mqss::createTransport<mqss::RabbitMqSimple>();
  mqss::Address address{uniqueName("queue.empty")};

  auto result = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 0ms, .ack_mode = mqss::AckMode::Auto});

  EXPECT_TRUE(hasStatusCode(result, mqss::StatusCode::Timeout));
}

// send(confirm=true) must fail if confirmation is unsupported.
TEST(RabbitMqTransportContract, SendConfirmIsUnsupported) {
  REQUIRE_RABBITMQ();

  auto transport = mqss::createTransport<mqss::RabbitMqSimple>();
  mqss::Address address{uniqueName("queue.send_confirm")};

  mqss::Envelope envelope;
  envelope.payload = "hello";

  auto status =
      transport->send(address, envelope, mqss::SendArgs{.confirm = true});

  EXPECT_EQ(status.code(), mqss::StatusCode::Unsupported);
}

// RabbitMQ transport advertises manual acknowledgment and requeue support.
TEST(RabbitMqTransportContract, ManualAckFeatureAdvertised) {
  // REQUIRE_RABBITMQ(); // No communication

  auto transport = mqss::createTransport<mqss::RabbitMqSimple>();
  auto fs = transport->features();

  EXPECT_TRUE(fs.has(mqss::Feature::ManualAck));
  EXPECT_TRUE(fs.has(mqss::Feature::Requeue));
  EXPECT_FALSE(fs.has(mqss::Feature::SendConfirm));
}

// A manually acknowledged message should not be received again.
TEST(RabbitMqTransportContract, ManualAckRoundTrip) {
  REQUIRE_RABBITMQ();

  auto transport = mqss::createTransport<mqss::RabbitMqSimple>();
  mqss::Address address{uniqueName("queue.manual_ack")};

  mqss::Envelope envelope;
  envelope.payload = "hello-ack";

  ASSERT_TRUE(transport->send(address, envelope).ok());

  auto r1 = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 1000ms, .ack_mode = mqss::AckMode::Manual});

  ASSERT_TRUE(r1.has_value());

  mqss::Message msg = std::move(*r1);
  EXPECT_EQ(msg.envelope.payload, "hello-ack");

  EXPECT_TRUE(msg.ack().ok());

  auto r2 = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 200ms, .ack_mode = mqss::AckMode::Auto});

  EXPECT_TRUE(hasStatusCode(r2, mqss::StatusCode::Timeout));
}

// Transport can be created with explicit RabbitMQ options.
TEST(RabbitMqTransportContract, CreateWithCustomOptions) {
  REQUIRE_RABBITMQ();

  mqss::TransportOptions<mqss::RabbitMqSimple> opts;
  opts.host = "127.0.0.1";
  opts.port = 5672;
  opts.username = "guest";
  opts.password = "guest";
  opts.vhost = "/";

  auto transport = mqss::createTransport<mqss::RabbitMqSimple>(opts);

  mqss::Address address{uniqueName("queue.custom_opts")};

  mqss::Envelope envelope;
  envelope.payload = "hello-options";

  auto status = transport->send(address, envelope);
  ASSERT_TRUE(status.ok()) << status.reason();

  auto result = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 1000ms, .ack_mode = mqss::AckMode::Auto});

  ASSERT_TRUE(result.has_value()) << result.error().reason();
  EXPECT_EQ(result->envelope.payload, "hello-options");
}
