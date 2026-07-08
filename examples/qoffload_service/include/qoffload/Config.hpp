// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// QOffload runtime configuration.

#include "ConfigDefaults.hpp"
#include "mqss/Config.hpp"
#include "mqss/Status.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
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

/// Logging severity threshold.
enum class LogLevel { Trace, Debug, Info, Warning, Error, Critical, Off };

/// File logging behavior.
enum class LogFileMode { Append, Truncate, Rotate };

/// Logging output and file-rotation settings.
struct LoggingConfig {
  bool enabled;
  bool console_enabled;
  bool file_enabled;
  LogLevel level;
  LogFileMode file_mode;
  std::size_t rotation_size;
  std::size_t rotation_files;
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

/// Converts the generated log-level default to its typed representation.
constexpr LogLevel defaultLogLevel(std::string_view value) {
  if (value == "trace")
    return LogLevel::Trace;
  if (value == "debug")
    return LogLevel::Debug;
  if (value == "warning" || value == "warn")
    return LogLevel::Warning;
  if (value == "error")
    return LogLevel::Error;
  if (value == "critical")
    return LogLevel::Critical;
  if (value == "off")
    return LogLevel::Off;

  return LogLevel::Info;
}

/// Converts the generated file-mode default to its typed representation.
constexpr LogFileMode defaultLogFileMode(std::string_view value) {
  if (value == "append")
    return LogFileMode::Append;
  if (value == "truncate")
    return LogFileMode::Truncate;

  return LogFileMode::Rotate;
}

/// Complete process configuration.
struct Config {
  RabbitMqConfig rabbitmq;
  RabbitMqConfig qrm_rabbitmq;
  QueueConfig queues;
  IdentityConfig identity;
  LoggingConfig logging;
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
      .logging =
          {
              .enabled = defaults::log_enabled,
              .console_enabled = defaults::log_console_enabled,
              .file_enabled = defaults::log_file_enabled,
              .level = defaultLogLevel(defaults::log_level),
              .file_mode = defaultLogFileMode(defaults::log_file_mode),
              .rotation_size = defaults::log_rotation_size,
              .rotation_files = defaults::log_rotation_files,
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

/// Parses a configured log level.
inline mqss::Result<LogLevel> parseLogLevel(std::string_view value) {
  if (value == "trace")
    return LogLevel::Trace;
  if (value == "debug")
    return LogLevel::Debug;
  if (value == "info")
    return LogLevel::Info;
  if (value == "warning" || value == "warn")
    return LogLevel::Warning;
  if (value == "error")
    return LogLevel::Error;
  if (value == "critical")
    return LogLevel::Critical;
  if (value == "off")
    return LogLevel::Off;

  return std::unexpected(
      mqss::Status::configuration("Invalid value for QOFFLOAD_LOG_LEVEL"));
}

/// Parses a configured file logging mode.
inline mqss::Result<LogFileMode> parseLogFileMode(std::string_view value) {
  if (value == "append")
    return LogFileMode::Append;
  if (value == "truncate")
    return LogFileMode::Truncate;
  if (value == "rotate")
    return LogFileMode::Rotate;

  return std::unexpected(
      mqss::Status::configuration("Invalid value for QOFFLOAD_LOG_FILE_MODE"));
}

/// Builds a log file path from a directory and file name.
inline std::string logPath(std::string_view log_dir,
                           std::string_view file_name) {
  return (std::filesystem::path(log_dir) / file_name).string();
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

  if (auto value = mqss::config::getEnvOr({"QOFFLOAD_LOG_ENABLED"},
                                          config.logging.enabled)) {
    config.logging.enabled = *value;
  } else {
    return value.error();
  }
  if (auto value = mqss::config::getEnvOr({"QOFFLOAD_LOG_CONSOLE_ENABLED"},
                                          config.logging.console_enabled)) {
    config.logging.console_enabled = *value;
  } else {
    return value.error();
  }
  if (auto value = mqss::config::getEnvOr({"QOFFLOAD_LOG_FILE_ENABLED"},
                                          config.logging.file_enabled)) {
    config.logging.file_enabled = *value;
  } else {
    return value.error();
  }
  if (const char *value = mqss::config::getEnv({"QOFFLOAD_LOG_LEVEL"})) {
    if (auto level = parseLogLevel(value)) {
      config.logging.level = *level;
    } else {
      return level.error();
    }
  }
  if (const char *value = mqss::config::getEnv({"QOFFLOAD_LOG_FILE_MODE"})) {
    if (auto mode = parseLogFileMode(value)) {
      config.logging.file_mode = *mode;
    } else {
      return mode.error();
    }
  }
  if (auto value = mqss::config::getEnvOr<std::size_t>(
          {"QOFFLOAD_LOG_ROTATION_SIZE"}, config.logging.rotation_size)) {
    config.logging.rotation_size = *value;
  } else {
    return value.error();
  }
  if (auto value = mqss::config::getEnvOr<std::size_t>(
          {"QOFFLOAD_LOG_ROTATION_FILES"}, config.logging.rotation_files)) {
    config.logging.rotation_files = *value;
  } else {
    return value.error();
  }

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
