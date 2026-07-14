// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "qoffload/QOffload.hpp"

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"

#include <chrono>
#include <utility>

namespace qoffload {
namespace {

using namespace std::chrono_literals;
using Messenger = mqss::Messenger<mqss::RabbitMqSimple, mqss::ProtoJson>;

bool isTimeout(const mqss::Status &status) {
  return status.code() == mqss::StatusCode::Timeout;
}

mqss::TransportOptions<mqss::RabbitMqSimple>
makeTransportOptions(const RabbitMqConfig &config) {
  mqss::TransportOptions<mqss::RabbitMqSimple> options;
  options.host = config.host;
  options.port = config.port;
  options.username = config.user;
  options.password = config.password;
  options.vhost = config.vhost;
  return options;
}

} // namespace

QOffload::QOffload(Config config, std::shared_ptr<spdlog::logger> logger)
    : config_(std::move(config)), logger_(std::move(logger)),
      forwarder_(config_.identity.instance_uid) {}

QOffload::~QOffload() { requestStop(); }

void QOffload::start() {
  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true)) {
    return;
  }

  logger_->info("Starting QOffload workers");
  startWorkers();
}

void QOffload::requestStop() {
  if (!running_.exchange(false)) {
    return;
  }

  logger_->info("Stopping QOffload workers");
  for (auto &worker : workers_) {
    worker.request_stop();
  }
  workers_.clear();
}

// Starts the background worker threads.
//
// The workers process client requests, forward quantum tasks to QRM,
// receive execution results, and serve management requests until the
// component is stopped.
void QOffload::startWorkers() {
  // Task-interface worker: receive client QuantumTask messages and forward
  // rewritten tasks to the QRM queue.
  workers_.emplace_back([this](std::stop_token stop) {
    Messenger local_messenger{makeTransportOptions(config_.rabbitmq)};
    Messenger qrm_messenger{makeTransportOptions(config_.qrm_rabbitmq)};

    while (!stop.stop_requested()) {
      auto received = local_messenger.receive<mqss::QuantumTask>(
          {config_.queues.request},
          mqss::ReceiveArgs{.timeout = 500ms, .ack_mode = mqss::AckMode::Auto});

      if (!received.has_value()) {
        if (isTimeout(received.error())) {
          continue;
        }
        logger_->error("Failed to receive QuantumTask: {}",
                       received.error().reason());
        continue;
      }

      auto qrm_task = forwarder_.forwardDirectTask(*received, tasks_, config_);
      auto status = qrm_messenger.send<mqss::QuantumTask>(
          {config_.queues.qrm_task}, qrm_task);
      if (!status.ok()) {
        logger_->error("Failed to forward QuantumTask: {}", status.reason());
      }
    }
  });

  // Control-interface worker: decode APIRequest messages into semantic
  // commands, execute them, and encode the resulting APIResponse.
  workers_.emplace_back([this](std::stop_token stop) {
    Messenger local_messenger{makeTransportOptions(config_.rabbitmq)};
    Messenger qrm_messenger{makeTransportOptions(config_.qrm_rabbitmq)};

    while (!stop.stop_requested()) {
      auto received = local_messenger.receive<mqss::APIRequest>(
          {config_.queues.control_request},
          mqss::ReceiveArgs{.timeout = 500ms, .ack_mode = mqss::AckMode::Auto});

      if (!received.has_value()) {
        if (isTimeout(received.error())) {
          continue;
        }
        logger_->error("Failed to receive control APIRequest: {}",
                       received.error().reason());
        continue;
      }

      // Control owns protocol adaptation, command execution, and response
      // encoding; this worker only handles message transport.
      auto outcome =
          control_.process(*received, tasks_, config_, resources_, forwarder_);

      if (outcome.outbound_task.has_value()) {
        auto task_status = qrm_messenger.send<mqss::QuantumTask>(
            {config_.queues.qrm_task}, *outcome.outbound_task);
        if (!task_status.ok()) {
          logger_->error("Failed to forward control QuantumTask: {}",
                         task_status.reason());
        }
      }

      if (!outcome.response.destination_queue().empty()) {
        auto response_status = local_messenger.send<mqss::APIResponse>(
            {outcome.response.destination_queue()}, outcome.response);
        if (!response_status.ok()) {
          logger_->error("Failed to send control APIResponse: {}",
                         response_status.reason());
        }
      }
    }
  });

  // QRM-result worker: correlate each QuantumResult with its originating
  // task, store control results, or send direct results back to the client.
  workers_.emplace_back([this](std::stop_token stop) {
    Messenger local_messenger{makeTransportOptions(config_.rabbitmq)};

    while (!stop.stop_requested()) {
      auto received = local_messenger.receive<mqss::QuantumResult>(
          {config_.queues.result},
          mqss::ReceiveArgs{.timeout = 500ms, .ack_mode = mqss::AckMode::Auto});

      if (!received.has_value()) {
        if (isTimeout(received.error())) {
          continue;
        }
        logger_->error("Failed to receive QuantumResult: {}",
                       received.error().reason());
        continue;
      }

      // Processing may consume the result into control state or produce a
      // remapped direct result for immediate delivery.
      auto outcome = forwarder_.processResult(std::move(*received), tasks_);
      if (outcome.kind != ResultProcessingKind::DirectResult ||
          !outcome.direct_result.has_value()) {
        continue;
      }

      auto status = local_messenger.send<mqss::QuantumResult>(
          {outcome.direct_result_destination}, *outcome.direct_result);
      if (!status.ok()) {
        logger_->error("Failed to send mapped QuantumResult: {}",
                       status.reason());
      }
    }
  });
}

} // namespace qoffload
