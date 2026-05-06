// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Tests for typed messaging using the RabbitMQ transport.
//
// Verifies typed send/receive behavior for protobuf binary and JSON encodings.
//
// Notes:
// - Uses unique queue names to avoid interference between test runs.
// - Assumes a RabbitMQ broker may be available on localhost:5672.
// - Tests are skipped if the broker is unavailable.

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <utility>

using namespace std::chrono_literals;

namespace {

enum class TypedFormat { ProtoBinary, ProtoJson };

template <TypedFormat F>
struct FormatTag;

template <>
struct FormatTag<TypedFormat::ProtoBinary> {
  using type = mqss::ProtoBinary;
};

template <>
struct FormatTag<TypedFormat::ProtoJson> {
  using type = mqss::ProtoJson;
};

class RabbitMqTypedTransport : public ::testing::TestWithParam<TypedFormat> {
protected:
  template <typename Fn>
  void withFormat(Fn &&fn) {
    switch (GetParam()) {
    case TypedFormat::ProtoBinary:
      fn(typename FormatTag<TypedFormat::ProtoBinary>::type{});
      break;
    case TypedFormat::ProtoJson:
      fn(typename FormatTag<TypedFormat::ProtoJson>::type{});
      break;
    }
  }

  static std::string uniqueName(const std::string &base) {
    static std::atomic<unsigned> seq{1};
    return base + "." + std::to_string(seq.fetch_add(1));
  }

  static std::optional<std::string> brokerUnavailableReason() {
    auto transport = mqss::createTransport<mqss::RabbitMqSimple>();
    mqss::Address address{uniqueName("queue.probe")};

    auto r = transport->receive(address, mqss::ReceiveArgs{
                                             .timeout = 0ms,
                                             .ack_mode = mqss::AckMode::Auto,
                                         });

    if (!r.has_value() && r.error().code() == mqss::StatusCode::Timeout) {
      return std::nullopt;
    }

    if (!r.has_value()) {
      return r.error().reason();
    }

    return std::nullopt;
  }
};

#define REQUIRE_RABBITMQ()                                                     \
  do {                                                                         \
    auto reason = brokerUnavailableReason();                                   \
    if (reason.has_value()) {                                                  \
      GTEST_SKIP() << "RabbitMQ broker unavailable: " << *reason;              \
    }                                                                          \
  } while (false)

// send<T>() -> receive<T>() basic typed roundtrip.
TEST_P(RabbitMqTypedTransport, SendReceive) {
  REQUIRE_RABBITMQ();

  withFormat([&](auto format_tag) {
    using Format = decltype(format_tag);

    mqss::Messenger<mqss::RabbitMqSimple, Format> messenger;

    const std::string queue_name = uniqueName("queue.tasks");

    mqss::QuantumTask task;
    task.set_task_id(119);
    task.set_n_qbits(5);
    task.set_n_shots(1024);
    task.set_result_destination("results.queue");

    auto send_st =
        messenger.template send<mqss::QuantumTask>({queue_name}, task);
    ASSERT_TRUE(send_st.ok()) << send_st.reason();

    auto res = messenger.template receive<mqss::QuantumTask>(
        {queue_name}, mqss::ReceiveArgs{
                          .timeout = 3000ms,
                          .ack_mode = mqss::AckMode::Auto,
                      });

    ASSERT_TRUE(res.has_value()) << res.error().reason();

    const auto &decoded = *res;
    EXPECT_EQ(decoded.task_id(), 119);
    EXPECT_EQ(decoded.n_qbits(), 5);
    EXPECT_EQ(decoded.n_shots(), 1024);
    EXPECT_EQ(decoded.result_destination(), "results.queue");
  });
}

// receiveExtended<T>() returns the decoded payload and original Message.
TEST_P(RabbitMqTypedTransport, SendReceiveExtended) {
  REQUIRE_RABBITMQ();

  withFormat([&](auto format_tag) {
    using Format = decltype(format_tag);

    mqss::Messenger<mqss::RabbitMqSimple, Format> messenger;

    const std::string queue_name = uniqueName("queue.tasks.extended");

    mqss::QuantumTask task;
    task.set_task_id(120);
    task.set_n_qbits(6);
    task.set_n_shots(2048);
    task.set_result_destination("results.extended.queue");

    auto send_st =
        messenger.template send<mqss::QuantumTask>({queue_name}, task);
    ASSERT_TRUE(send_st.ok()) << send_st.reason();

    auto res = messenger.template receiveExtended<mqss::QuantumTask>(
        {queue_name}, mqss::ReceiveArgs{
                          .timeout = 3000ms,
                          .ack_mode = mqss::AckMode::Manual,
                      });

    ASSERT_TRUE(res.has_value()) << res.error().reason();

    auto [decoded, raw] = std::move(*res);

    EXPECT_EQ(decoded.task_id(), 120);
    EXPECT_EQ(decoded.n_qbits(), 6);
    EXPECT_EQ(decoded.n_shots(), 2048);
    EXPECT_EQ(decoded.result_destination(), "results.extended.queue");

    auto ack_st = raw.ack();
    EXPECT_TRUE(ack_st.ok() || ack_st.code() == mqss::StatusCode::Unsupported)
        << ack_st.reason();
  });
}

INSTANTIATE_TEST_SUITE_P(Formats, RabbitMqTypedTransport,
                         ::testing::Values(TypedFormat::ProtoBinary,
                                           TypedFormat::ProtoJson),
                         [](const ::testing::TestParamInfo<TypedFormat> &info) {
                           switch (info.param) {
                           case TypedFormat::ProtoBinary:
                             return "ProtoBinary";
                           case TypedFormat::ProtoJson:
                             return "ProtoJson";
                           }
                           return "Unknown";
                         });

} // namespace
