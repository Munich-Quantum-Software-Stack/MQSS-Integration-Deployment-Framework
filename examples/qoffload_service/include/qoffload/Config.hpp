// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// QOffload runtime configuration.

#include "ConfigDefaults.hpp"
#include "mqss/Config.hpp"
#include "mqss/Status.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace qoffload {

/// Identifies the QOffload configuration and its process-wide storage.
struct ConfigTag {};

/// RabbitMQ connection settings.
struct RabbitMqConfig {
  std::string host;
  int port{};
  std::string user;
  std::string password;
  std::string vhost;
};

/// Queue names used by the Task, Control, and QRM interfaces.
struct QueueConfig {
  std::string request;
  std::string control_request;
  std::string result;
  std::string qrm_task;
};

/// QOffload instance identity.
struct IdentityConfig {
  std::uint64_t instance_uid{};
  std::string user_identity;
};

/// Runtime paths used by QOffload.
struct PathConfig {
  std::string runtime_dir;
  std::string log_dir;
  std::string state_file;
};

/// Runtime policy options.
struct PolicyConfig {
  bool hpc_node{};
  bool resume_state{};
};

/// Complete process configuration.
struct Config {
  RabbitMqConfig rabbitmq;
  RabbitMqConfig qrm_rabbitmq;
  QueueConfig queues;
  IdentityConfig identity;
  PathConfig paths;
  PolicyConfig policy;
};

/// Creates the QOffload configuration from generated defaults.
inline Config makeDefaultConfig(ConfigTag) {
  return Config{
      .rabbitmq =
          {
              .host = std::string(defaults::amqp_host),
              .port = defaults::amqp_port,
              .user = std::string(defaults::amqp_user),
              .password = std::string(defaults::amqp_password),
              .vhost = std::string(defaults::amqp_vhost),
          },
      .qrm_rabbitmq =
          {
              .host = std::string(defaults::qrm_amqp_host),
              .port = defaults::qrm_amqp_port,
              .user = std::string(defaults::qrm_amqp_user),
              .password = std::string(defaults::qrm_amqp_password),
              .vhost = std::string(defaults::qrm_amqp_vhost),
          },
      .queues =
          {
              .request = std::string(defaults::request_queue),
              .control_request = std::string(defaults::control_request_queue),
              .result = std::string(defaults::result_queue),
              .qrm_task = std::string(defaults::qrm_task_queue),
          },
      .identity =
          {
              .instance_uid = defaults::instance_uid,
              .user_identity = std::string(defaults::user_identity),
          },
      .paths =
          {
              .runtime_dir = std::string(defaults::runtime_dir),
              .log_dir = std::string(defaults::log_dir),
              .state_file = std::string(defaults::state_file),
          },
      .policy =
          {
              .hpc_node = defaults::hpc_node,
              .resume_state = defaults::resume_state,
          },
  };
}

/// Applies QOffload environment-variable overrides.
inline mqss::Status applyEnvironment(ConfigTag, Config &config) {
  config.rabbitmq.host =
      mqss::config::getEnvOr({"QOFFLOAD_AMQP_HOST"}, config.rabbitmq.host);
  if (auto value = mqss::config::getEnvOr<int>({"QOFFLOAD_AMQP_PORT"},
                                               config.rabbitmq.port)) {
    config.rabbitmq.port = *value;
  } else {
    return value.error();
  }
  config.rabbitmq.user =
      mqss::config::getEnvOr({"QOFFLOAD_AMQP_USER"}, config.rabbitmq.user);
  config.rabbitmq.password = mqss::config::getEnvOr({"QOFFLOAD_AMQP_PASSWORD"},
                                                    config.rabbitmq.password);
  config.rabbitmq.vhost =
      mqss::config::getEnvOr({"QOFFLOAD_AMQP_VHOST"}, config.rabbitmq.vhost);

  config.qrm_rabbitmq.host =
      mqss::config::getEnvOr({"QRM_AMQP_HOST"}, config.qrm_rabbitmq.host);
  if (auto value = mqss::config::getEnvOr<int>({"QRM_AMQP_PORT"},
                                               config.qrm_rabbitmq.port)) {
    config.qrm_rabbitmq.port = *value;
  } else {
    return value.error();
  }
  config.qrm_rabbitmq.user =
      mqss::config::getEnvOr({"QRM_AMQP_USER"}, config.qrm_rabbitmq.user);
  config.qrm_rabbitmq.password = mqss::config::getEnvOr(
      {"QRM_AMQP_PASSWORD"}, config.qrm_rabbitmq.password);
  config.qrm_rabbitmq.vhost =
      mqss::config::getEnvOr({"QRM_AMQP_VHOST"}, config.qrm_rabbitmq.vhost);

  config.queues.request =
      mqss::config::getEnvOr({"QOFFLOAD_REQUEST_QUEUE"}, config.queues.request);
  config.queues.control_request = mqss::config::getEnvOr(
      {"QOFFLOAD_CONTROL_REQUEST_QUEUE", "QOFFLOAD_API_REQUEST_QUEUE"},
      config.queues.control_request);
  config.queues.result =
      mqss::config::getEnvOr({"QOFFLOAD_RESULT_QUEUE"}, config.queues.result);
  config.queues.qrm_task =
      mqss::config::getEnvOr({"QRM_TASK_QUEUE"}, config.queues.qrm_task);

  if (auto value = mqss::config::getEnvOr<std::uint64_t>(
          {"QOFFLOAD_INSTANCE_UID", "QOFFLOAD_DAEMON_UID"},
          config.identity.instance_uid)) {
    config.identity.instance_uid = *value;
  } else {
    return value.error();
  }
  config.identity.user_identity = mqss::config::getEnvOr(
      {"QOFFLOAD_USER_IDENTITY"}, config.identity.user_identity);

  config.paths.runtime_dir = mqss::config::getEnvOr({"QOFFLOAD_RUNTIME_DIR"},
                                                    config.paths.runtime_dir);
  config.paths.log_dir =
      mqss::config::getEnvOr({"QOFFLOAD_LOG_DIR"}, config.paths.log_dir);
  config.paths.state_file =
      mqss::config::getEnvOr({"QOFFLOAD_STATE_FILE"}, config.paths.state_file);

  if (auto value = mqss::config::getEnvOr({"QOFFLOAD_HPC_NODE"},
                                          config.policy.hpc_node)) {
    config.policy.hpc_node = *value;
  } else {
    return value.error();
  }
  if (auto value = mqss::config::getEnvOr({"QOFFLOAD_RESUME_STATE"},
                                          config.policy.resume_state)) {
    config.policy.resume_state = *value;
  } else {
    return value.error();
  }

  return mqss::Status::success();
}

/// Applies QOffload command-line overrides.
inline mqss::Status applyCommandLine(ConfigTag, Config &, int, char **) {
  return mqss::Status::success();
}

/// Loads configuration using defaults, environment, then command-line options.
inline mqss::Result<Config> loadConfig(int argc, char **argv) {
  return mqss::config::loadConfig<Config>(ConfigTag{}, argc, argv);
}

/// Stores the process-wide immutable configuration.
inline mqss::Status initConfig(Config config) {
  return mqss::config::initConfig(ConfigTag{}, std::move(config));
}

/// Returns the initialized process-wide configuration.
inline mqss::Result<std::reference_wrapper<const Config>> getConfig() {
  return mqss::config::getConfig<Config>(ConfigTag{});
}

} // namespace qoffload
