// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "Config.hpp"
#include "Logger.hpp"

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"

#include <chrono>
#include <filesystem>
#include <string>

int main(int argc, char **argv) {
  mqss::examples::qrm_workflow::initConfig(
      mqss::examples::qrm_workflow::loadConfig(argc, argv));

  auto config = mqss::examples::qrm_workflow::getConfig();

  auto logger = mqss::examples::qrm_workflow::makeLogger(
      config.logging.daemon_logger, config.logging.daemon_log);

  // Use a process-specific default logger to avoid passing logger objects.
  spdlog::set_default_logger(logger);

  spdlog::info("Starting daemon");

  mqss::TransportOptions<mqss::RabbitMqSimple> opts;
  opts.host = std::string(config.rabbitmq.host);
  opts.port = config.rabbitmq.port;
  opts.username = std::string(config.rabbitmq.user);
  opts.password = std::string(config.rabbitmq.password);

  mqss::Messenger<mqss::RabbitMqSimple, mqss::ProtoJson> messenger(opts);

  mqss::QuantumTask task{};
  task.set_task_id(111);
  task.set_n_qbits(6);
  task.set_n_shots(2048);
  task.set_optimisation_level(1);
  task.set_result_destination(std::string(config.queues.results));
  task.set_preferred_qpu(
      "iqm"); // Either iqm, fermioniq, ionq, oqc, quantinuum, qci

  auto circuit_file_path = std::filesystem::path(config.paths.benchmark_dir) /
                           "DoubleCnotCancellation.cpp";

  task.add_circuit_files(circuit_file_path);
  task.set_circuit_file_type("cpp");

  auto send_st = messenger.send<mqss::QuantumTask>(
      {std::string(config.queues.scheduler)}, task);

  if (!send_st.ok()) {
    spdlog::error("Message could not be sent: {}", send_st.reason());
    return 1;
  }

  spdlog::info("Task sent");

  auto res = messenger.receive<mqss::QuantumTask>(
      {task.result_destination()},
      mqss::ReceiveArgs{
          .timeout = std::chrono::milliseconds(60000),
          .ack_mode = mqss::AckMode::Auto,
      });

  if (!res.has_value()) {
    spdlog::error("Failed to receive result: {}", res.error().reason());
    return 1;
  }

  spdlog::info("Task received by daemon queue");

  const auto &decoded = *res;

  spdlog::info("Decoded task id: {}", decoded.task_id());
  spdlog::info("New circuit files dump:");

  for (const auto &circuit : decoded.circuit_files()) {
    spdlog::info("{}", circuit);
  }

  // use decoded.task_id(), decoded.n_qbits(), etc.

  return 0;
}
