// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Contract tests for the Transport API using the MPI transport.
//
// Verifies:
// - point-to-point send/receive between MPI ranks
// - timeout semantics
// - ordering guarantees for same source/tag
// - unsupported features (send confirmation)
// - invalid address handling

#include "mqss/Status.hpp"
#include "mqss/transport/MpiTransport.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>

using namespace std::chrono_literals;

namespace {

template <class T>
bool hasStatusCode(const mqss::Result<T> &result, mqss::StatusCode code) {
  return !result.has_value() && result.error().code() == code;
}

mqss::TransportOptions<mqss::Mpi> mpiOptions(int tag = 100) {
  mqss::TransportOptions<mqss::Mpi> options;
  options.communicator = MPI_COMM_WORLD;
  options.tag = tag;
  options.initialize_mpi = false;
  return options;
}

} // namespace

// send() on one rank -> receive() on another rank.
TEST(MpiTransportContract, SendThenReceiveBetweenRanks) {
  int rank = -1;
  int size = 0;
  auto opts = mpiOptions();

  MPI_Comm_rank(opts.communicator, &rank);
  MPI_Comm_size(opts.communicator, &size);

  ASSERT_GE(size, 2);

  auto transport = mqss::createTransport<mqss::Mpi>(mpiOptions());

  if (rank == 0) {
    mqss::Envelope envelope;
    envelope.payload = "hello";

    EXPECT_TRUE(transport->send(mqss::Address{"1"}, envelope).ok());
  }

  if (rank == 1) {
    auto result = transport->receive(
        mqss::Address{"0"},
        mqss::ReceiveArgs{.timeout = 1s, .ack_mode = mqss::AckMode::Auto});

    ASSERT_TRUE(result);
    EXPECT_EQ(result->envelope.payload, "hello");
  }
}

// receive() with zero timeout acts as non-blocking poll.
TEST(MpiTransportContract, ReceiveReturnsTimeoutForZeroTimeout) {
  int rank = -1;
  auto opts = mpiOptions();

  MPI_Comm_rank(opts.communicator, &rank);

  auto transport = mqss::createTransport<mqss::Mpi>(mpiOptions());

  auto result = transport->receive(
      mqss::Address{std::to_string(rank == 0 ? 1 : 0)},
      mqss::ReceiveArgs{.timeout = 0ms, .ack_mode = mqss::AckMode::Auto});

  EXPECT_TRUE(hasStatusCode(result, mqss::StatusCode::Timeout));
}

// Messages from the same sender preserve send order.
TEST(MpiTransportContract, MessagesAreReceivedInSendOrder) {
  int rank = -1;
  int size = 0;
  auto opts = mpiOptions();

  MPI_Comm_rank(opts.communicator, &rank);
  MPI_Comm_size(opts.communicator, &size);

  ASSERT_GE(size, 2);

  auto transport = mqss::createTransport<mqss::Mpi>(mpiOptions());

  if (rank == 0) {
    mqss::Envelope first;
    first.payload = "first";

    mqss::Envelope second;
    second.payload = "second";

    ASSERT_TRUE(transport->send(mqss::Address{"1"}, first).ok());
    ASSERT_TRUE(transport->send(mqss::Address{"1"}, second).ok());
  }

  if (rank == 1) {
    auto r1 = transport->receive(
        mqss::Address{"0"},
        mqss::ReceiveArgs{.timeout = 1s, .ack_mode = mqss::AckMode::Auto});

    auto r2 = transport->receive(
        mqss::Address{"0"},
        mqss::ReceiveArgs{.timeout = 1s, .ack_mode = mqss::AckMode::Auto});

    ASSERT_TRUE(r1);
    ASSERT_TRUE(r2);

    EXPECT_EQ(r1->envelope.payload, "first");
    EXPECT_EQ(r2->envelope.payload, "second");
  }
}

// Unsupported features: MPI doesn't support send confirmation
TEST(MpiTransportContract, SendConfirmIsUnsupported) {
  int rank = -1;
  auto opts = mpiOptions();

  MPI_Comm_rank(opts.communicator, &rank);

  auto transport = mqss::createTransport<mqss::Mpi>(mpiOptions());

  mqss::Envelope envelope;
  envelope.payload = "hello";

  auto status = transport->send(mqss::Address{std::to_string(rank)}, envelope,
                                mqss::SendArgs{.confirm = true});

  EXPECT_EQ(status.code(), mqss::StatusCode::Unsupported);
}

// Invalid address handling
TEST(MpiTransportContract, InvalidAddressIsRejected) {
  auto transport = mqss::createTransport<mqss::Mpi>(mpiOptions());

  mqss::Envelope envelope;
  envelope.payload = "hello";

  auto status = transport->send(mqss::Address{"not-a-rank"}, envelope);

  EXPECT_EQ(status.code(), mqss::StatusCode::InvalidArgument);
}

int main(int argc, char **argv) {
  int provided = MPI_THREAD_SINGLE;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  MPI_Finalize();
  return result;
}
