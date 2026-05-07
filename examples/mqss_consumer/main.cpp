// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"

#include <chrono>
#include <iostream>
#include <string>

using namespace std::chrono_literals;

int main() {
  mqss::Messenger<mqss::InMemory, mqss::ProtoBinary> messenger;

  mqss::QuantumTask task;
  task.set_task_id(119);
  task.set_n_qbits(5);
  task.set_n_shots(1024);
  task.set_result_destination("results.queue");

  auto send_st = messenger.send<mqss::QuantumTask>({"tasks"}, task);
  if (!send_st.ok()) {
    std::cerr << "send failed: " << send_st.reason() << '\n';
    return 1;
  }

  auto res = messenger.receive<mqss::QuantumTask>(
      {"tasks"}, mqss::ReceiveArgs{
                     .timeout = 0ms,
                     .ack_mode = mqss::AckMode::Auto,
                 });

  if (!res.has_value()) {
    std::cerr << "receive failed: " << res.error().reason() << '\n';
    return 1;
  }

  const auto &decoded = *res;

  if (decoded.task_id() != 119 || decoded.n_qbits() != 5 ||
      decoded.n_shots() != 1024 ||
      decoded.result_destination() != "results.queue") {
    std::cerr << "roundtrip verification failed\n";
    return 1;
  }

  std::cout << "MQSS in-memory roundtrip succeeded\n";
  return 0;
}
