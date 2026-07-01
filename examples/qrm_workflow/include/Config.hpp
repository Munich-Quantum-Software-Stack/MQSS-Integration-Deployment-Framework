// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Configuration interface for the QRM workflow example.
//
// Defines the configuration model and provides access to the process-wide
// configuration instance.

#pragma once

#include <string>

namespace mqss::examples::qrm_workflow {

struct RabbitMqConfig {
  std::string host;
  int port;
  std::string user;
  std::string password;
  std::string vhost;
};

struct QueueConfig {
  std::string scheduler;
  std::string compiler;
  std::string results;
  std::string submitter;
};

struct LoggingConfig {
  std::string log_dir;

  std::string daemon_logger;
  std::string scheduler_logger;
  std::string compiler_logger;
  std::string submitter_logger;

  std::string daemon_log;
  std::string scheduler_log;
  std::string compiler_log;
  std::string submitter_log;
};

struct ToolConfig {
  std::string cudaq_quake;
  std::string mqss_cudaq_opt;
  std::string cudaq_opt;
  std::string cudaq_translate;
};

struct PathConfig {
  std::string benchmark_dir;
  std::string qdmi_device_objs_dir;
};

struct QDMIDevices{
  std::string qdmi_device_obj;
  std::string qdmi_device_prefix;
};

struct Config {
  RabbitMqConfig rabbitmq;
  QueueConfig queues;
  LoggingConfig logging;
  ToolConfig tools;
  PathConfig paths;
  QDMIDevices devices;
};

// Build a configuration from defaults and environment overrides.
Config loadConfig(int argc, char **argv);

// Store the process-wide configuration.
void initConfig(Config config);

// Return the initialized process-wide configuration.
const Config &getConfig();

} // namespace mqss::examples::qrm_workflow
