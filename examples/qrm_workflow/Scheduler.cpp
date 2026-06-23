// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "Config.hpp"
#include "Logger.hpp"

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"

#include <chrono>
#include <string>

int main(int argc, char **argv) {
  mqss::examples::qrm_workflow::initConfig(
      mqss::examples::qrm_workflow::loadConfig(argc, argv));

  auto config = mqss::examples::qrm_workflow::getConfig();

  auto logger = mqss::examples::qrm_workflow::makeLogger(
      config.logging.scheduler_logger, config.logging.scheduler_log);

  // Use a process-specific default logger to avoid passing logger objects.
  spdlog::set_default_logger(logger);

  spdlog::info("Starting MQSS scheduler");

  mqss::TransportOptions<mqss::RabbitMqSimple> opts;
  opts.host = std::string(config.rabbitmq.host);
  opts.port = config.rabbitmq.port;
  opts.username = std::string(config.rabbitmq.user);
  opts.password = std::string(config.rabbitmq.password);

  mqss::Messenger<mqss::RabbitMqSimple, mqss::ProtoJson> messenger(opts);

  while (true) {
    spdlog::info("Waiting for a new job");

    auto res = messenger.receive<mqss::QuantumTask>(
        {std::string(config.queues.scheduler)},
        mqss::ReceiveArgs{
            .timeout = std::chrono::milliseconds(5000),
            .ack_mode = mqss::AckMode::Auto,
        });

    if (!res.has_value()) {
      // Timeout is normal - just keep polling.
      if (res.error().code() == mqss::StatusCode::Timeout)
        continue;

      spdlog::error("Receive error: {}", res.error().reason());
      continue;
    }

    auto task = *res;

    spdlog::info("Received task: {}", task.task_id());
    spdlog::info("Forwarding task to compiler: {}", task.task_id());

    auto send_st = messenger.send<mqss::QuantumTask>(
        {std::string(config.queues.compiler)}, task);

    if (!send_st.ok()) {
      spdlog::error("Failed to send task: {}", send_st.reason());
      continue;
    }

    spdlog::info("Task forwarded to compiler: {}", task.task_id());
  }

  return 0;
}
