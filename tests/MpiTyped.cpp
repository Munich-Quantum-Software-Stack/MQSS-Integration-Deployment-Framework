// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Tests for typed messaging using the MPI transport.
//
// Verifies typed send/receive behavior for protobuf binary and JSON encodings.
//
// Notes:
// - Tests are intended to run under mpiexec with at least 2 ranks.
// - Rank 0 sends messages.
// - Rank 1 receives messages.
// - Other ranks, if present, do not participate.

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/transport/MpiTransport.hpp"

#include <gtest/gtest.h>
#include <mpi.h>

#include <chrono>
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

class MpiTypedTransport : public ::testing::TestWithParam<TypedFormat> {
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

  static mqss::TransportOptions<mqss::Mpi> mpiOptions(int tag = 100) {
    mqss::TransportOptions<mqss::Mpi> options;
    options.communicator = MPI_COMM_WORLD;
    options.tag = tag;
    options.initialize_mpi = false;
    return options;
  }
};

// send<T>() on rank 0 -> receive<T>() on rank 1.
TEST_P(MpiTypedTransport, SendReceive) {
  int rank = -1;
  int size = 0;
  auto opts = mpiOptions();

  MPI_Comm_rank(opts.communicator, &rank);
  MPI_Comm_size(opts.communicator, &size);

  ASSERT_GE(size, 2);

  withFormat([&](auto format_tag) {
    using Format = decltype(format_tag);

    mqss::Messenger<mqss::Mpi, Format> messenger(mpiOptions(200));

    if (rank == 0) {
      mqss::QuantumTask task;
      task.set_task_id(119);
      task.set_n_qbits(5);
      task.set_n_shots(1024);
      task.set_result_destination("results.rank.1");

      auto send_st = messenger.template send<mqss::QuantumTask>({"1"}, task);
      ASSERT_TRUE(send_st.ok()) << send_st.reason();
    }

    if (rank == 1) {
      auto res = messenger.template receive<mqss::QuantumTask>(
          {"0"}, mqss::ReceiveArgs{
                     .timeout = 3000ms,
                     .ack_mode = mqss::AckMode::Auto,
                 });

      ASSERT_TRUE(res.has_value()) << res.error().reason();

      const auto &decoded = *res;
      EXPECT_EQ(decoded.task_id(), 119);
      EXPECT_EQ(decoded.n_qbits(), 5);
      EXPECT_EQ(decoded.n_shots(), 1024);
      EXPECT_EQ(decoded.result_destination(), "results.rank.1");
    }
  });
}

// receiveExtended<T>() returns the decoded payload and original Message.
TEST_P(MpiTypedTransport, SendReceiveExtended) {
  int rank = -1;
  int size = 0;
  auto opts = mpiOptions();

  MPI_Comm_rank(opts.communicator, &rank);
  MPI_Comm_size(opts.communicator, &size);

  ASSERT_GE(size, 2);

  withFormat([&](auto format_tag) {
    using Format = decltype(format_tag);

    mqss::Messenger<mqss::Mpi, Format> messenger(mpiOptions(201));

    if (rank == 0) {
      mqss::QuantumTask task;
      task.set_task_id(120);
      task.set_n_qbits(6);
      task.set_n_shots(2048);
      task.set_result_destination("results.rank.1.extended");

      auto send_st = messenger.template send<mqss::QuantumTask>({"1"}, task);
      ASSERT_TRUE(send_st.ok()) << send_st.reason();
    }

    if (rank == 1) {
      auto res = messenger.template receiveExtended<mqss::QuantumTask>(
          {"0"}, mqss::ReceiveArgs{
                     .timeout = 3000ms,
                     .ack_mode = mqss::AckMode::Manual,
                 });

      ASSERT_TRUE(res.has_value()) << res.error().reason();

      auto [decoded, raw] = std::move(*res);

      EXPECT_EQ(decoded.task_id(), 120);
      EXPECT_EQ(decoded.n_qbits(), 6);
      EXPECT_EQ(decoded.n_shots(), 2048);
      EXPECT_EQ(decoded.result_destination(), "results.rank.1.extended");

      auto ack_st = raw.ack();
      EXPECT_EQ(ack_st.code(), mqss::StatusCode::Unsupported);
    }
  });
}

INSTANTIATE_TEST_SUITE_P(Formats, MpiTypedTransport,
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

int main(int argc, char **argv) {
  int provided = MPI_THREAD_SINGLE;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  MPI_Finalize();
  return result;
}
