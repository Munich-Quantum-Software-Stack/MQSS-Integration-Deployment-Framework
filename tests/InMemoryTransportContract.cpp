// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Contract tests for the Transport API using the in-memory transport.
//
// Verifies:
// - basic send/receive behavior
// - timeout semantics
// - unsupported features (send confirmation)
// - shutdown behavior and unblocking of receive()

#include "mqss/Status.hpp"
#include <mqss/transport/InMemoryTransport.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

namespace {
// Helper: check if Result<T> contains a specific Status code.
template <class T>
bool hasStatusCode(const mqss::Result<T> &result, mqss::StatusCode code) {
  return !result && result.error().code() == code;
}

} // namespace

// send() -> receive() basic roundtrip.
TEST(InMemoryTransportContract, SendThenReceive) {
  auto transport = mqss::createTransport<mqss::InMemory>();

  mqss::Address address{"queue.A"};

  mqss::Envelope envelope;
  envelope.payload = "hello";

  EXPECT_TRUE(transport->send(address, envelope).ok());

  auto result = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 100ms, .ack_mode = mqss::AckMode::Auto});

  ASSERT_TRUE(result);
  EXPECT_EQ(result->envelope.payload, "hello");
}

// receive() with zero timeout acts as non-blocking poll.
TEST(InMemoryTransportContract, ReceiveReturnsTimeoutForZeroTimeout) {
  auto transport = mqss::createTransport<mqss::InMemory>();

  mqss::Address address{"queue.empty"};

  auto result = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 0ms, .ack_mode = mqss::AckMode::Auto});

  EXPECT_TRUE(hasStatusCode(result, mqss::StatusCode::Timeout));
}

TEST(InMemoryTransportContract, ReceiveReturnsTimeoutAfterWait) {
  auto transport = mqss::createTransport<mqss::InMemory>();

  mqss::Address address{"queue.empty"};

  auto result = transport->receive(
      address,
      mqss::ReceiveArgs{.timeout = 20ms, .ack_mode = mqss::AckMode::Auto});

  EXPECT_TRUE(hasStatusCode(result, mqss::StatusCode::Timeout));
}

TEST(InMemoryTransportContract, ReceiveBlocksUntilMessageArrives) {
  auto transport = mqss::createTransport<mqss::InMemory>();

  mqss::Address address{"queue.wait"};

  auto future = std::async(std::launch::async,
                           [&] { return transport->receive(address); });

  std::this_thread::sleep_for(20ms);

  mqss::Envelope envelope;
  envelope.payload = "delayed";

  ASSERT_TRUE(transport->send(address, envelope).ok());

  auto result = future.get();

  ASSERT_TRUE(result);
  EXPECT_EQ(result->envelope.payload, "delayed");
}

TEST(InMemoryTransportContract, MessagesAreReceivedInSendOrder) {
  auto transport = mqss::createTransport<mqss::InMemory>();

  mqss::Address address{"queue.order"};

  mqss::Envelope first;
  first.payload = "first";

  mqss::Envelope second;
  second.payload = "second";

  ASSERT_TRUE(transport->send(address, first).ok());
  ASSERT_TRUE(transport->send(address, second).ok());

  auto r1 = transport->receive(address);
  auto r2 = transport->receive(address);

  ASSERT_TRUE(r1);
  ASSERT_TRUE(r2);

  EXPECT_EQ(r1->envelope.payload, "first");
  EXPECT_EQ(r2->envelope.payload, "second");
}

// send(confirm=true) must fail if feature is unsupported.
TEST(InMemoryTransportContract, SendConfirmIsUnsupported) {
  auto transport = mqss::createTransport<mqss::InMemory>();
  mqss::Address address{"queue.A"};

  mqss::Envelope envelope;
  envelope.payload = "hello";

  auto status =
      transport->send(address, envelope, mqss::SendArgs{.confirm = true});

  EXPECT_EQ(status.code(), mqss::StatusCode::Unsupported);
}

// Destroying the transport must unblock a waiting receive().
TEST(InMemoryTransportContract, DestroyCompletesPendingReceive) {
  std::future<mqss::Result<mqss::Message>> future;

  {
    auto transport = mqss::createTransport<mqss::InMemory>();
    mqss::Address address{"queue.wait"};

    future = std::async(std::launch::async,
                        [&] { return transport->receive(address); });

    std::this_thread::sleep_for(20ms);
  }

  ASSERT_EQ(future.wait_for(500ms), std::future_status::ready);

  auto result = future.get();
  EXPECT_TRUE(hasStatusCode(result, mqss::StatusCode::Unavailable));
}

/// Multiple concurrent senders and receivers should deliver all messages once.
TEST(InMemoryTransportContract, ConcurrentSendAndReceive) {
  auto transport = mqss::createTransport<mqss::InMemory>();
  mqss::Address a{"queue.concurrent"};

  constexpr int sender_count = 4;
  constexpr int messages_per_sender = 25;
  constexpr int total_messages = sender_count * messages_per_sender;

  std::atomic<int> received_count{0};

  std::vector<std::future<void>> senders;
  std::vector<std::future<void>> receivers;

  // Start receivers first so some receive() calls block before messages arrive.
  for (int i = 0; i < total_messages; ++i) {
    receivers.push_back(std::async(std::launch::async, [&] {
      auto result = transport->receive(
          a, mqss::ReceiveArgs{.timeout = 1s, .ack_mode = mqss::AckMode::Auto});

      ASSERT_TRUE(result);
      received_count.fetch_add(1);
    }));
  }

  // Send messages concurrently from several producers.
  for (int s = 0; s < sender_count; ++s) {
    senders.push_back(std::async(std::launch::async, [&, s] {
      for (int i = 0; i < messages_per_sender; ++i) {
        mqss::Envelope e;
        e.payload = "sender-" + std::to_string(s) + "-msg-" + std::to_string(i);

        EXPECT_TRUE(transport->send(a, e).ok());
      }
    }));
  }

  for (auto &sender : senders) {
    sender.get();
  }

  for (auto &receiver : receivers) {
    receiver.get();
  }

  EXPECT_EQ(received_count.load(), total_messages);
}
