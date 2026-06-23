// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "Config.hpp"

#include "ConfigDefaults.hpp"

#include <cassert>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

namespace mqss::examples::qrm_workflow {

namespace {

Config config;
bool config_initialized = false;

std::string getEnvOr(const char *name, std::string_view fallback) {
  if (const char *value = std::getenv(name)) {
    return value;
  }

  return std::string(fallback);
}

int getEnvOr(const char *name, int fallback) {
  if (const char *value = std::getenv(name)) {
    int result = fallback;
    std::string_view sv(value);

    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
    if (ec == std::errc{} && ptr == sv.data() + sv.size()) {
      return result;
    }
  }

  return fallback;
}

std::string logPath(std::string_view log_dir, std::string_view file_name) {
  return (std::filesystem::path(log_dir) / file_name).string();
}

} // namespace

Config loadConfig(int argc, char **argv) {
  const auto log_dir = getEnvOr("QRM_LOG_DIR", defaults::log_dir);

  Config config{
      .rabbitmq =
          {
              .host = getEnvOr("QRM_AMQP_HOST", defaults::amqp_host),
              .port = getEnvOr("QRM_AMQP_PORT", defaults::amqp_port),
              .user = getEnvOr("QRM_AMQP_USER", defaults::amqp_user),
              .password =
                  getEnvOr("QRM_AMQP_PASSWORD", defaults::amqp_password),
              .vhost = getEnvOr("QRM_AMQP_VHOST", defaults::amqp_vhost),
          },

      .queues =
          {
              .scheduler =
                  getEnvOr("QRM_SCHEDULER_QUEUE", defaults::scheduler_queue),
              .compiler =
                  getEnvOr("QRM_COMPILER_QUEUE", defaults::compiler_queue),
              .results = getEnvOr("QRM_RESULTS_QUEUE", defaults::results_queue),
          },

      .logging =
          {
              .log_dir = log_dir,

              .daemon_logger = "mqss::Daemon",
              .scheduler_logger = "mqss::Scheduler",
              .compiler_logger = "mqss::Compiler",

              .daemon_log = logPath(log_dir, "daemon.log"),
              .scheduler_log = logPath(log_dir, "scheduler.log"),
              .compiler_log = logPath(log_dir, "compiler.log"),
          },

      .tools =
          {
              .cudaq_quake = getEnvOr("QRM_CUDAQ_QUAKE", defaults::cudaq_quake),
              .mqss_cudaq_opt =
                  getEnvOr("QRM_MQSS_CUDAQ_OPT", defaults::mqss_cudaq_opt),
              .cudaq_opt = getEnvOr("QRM_CUDAQ_OPT", defaults::cudaq_opt),
              .cudaq_translate =
                  getEnvOr("QRM_CUDAQ_TRANSLATE", defaults::cudaq_translate),
          },

      .paths =
          {
              .benchmark_dir =
                  getEnvOr("QRM_BENCHMARK_DIR", defaults::benchmark_dir),
          },
  };

  // Integrate CLI11 here when runtime CLI overrides are required.
  (void)argc;
  (void)argv;

  return config;
}

void initConfig(Config new_config) {
  assert(!config_initialized &&
         "QRM workflow configuration already initialized");

  config = std::move(new_config);
  config_initialized = true;
}

const Config &getConfig() {
  assert(config_initialized && "QRM workflow configuration not initialized");

  return config;
}

} // namespace mqss::examples::qrm_workflow
