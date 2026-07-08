// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Logging utilities for QOffload.

#pragma once

#include "mqss/Status.hpp"
#include "qoffload/Config.hpp"

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace qoffload {

/// Converts the application log level to the corresponding spdlog level.
inline spdlog::level::level_enum toSpdlogLevel(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
    return spdlog::level::trace;
  case LogLevel::Debug:
    return spdlog::level::debug;
  case LogLevel::Info:
    return spdlog::level::info;
  case LogLevel::Warning:
    return spdlog::level::warn;
  case LogLevel::Error:
    return spdlog::level::err;
  case LogLevel::Critical:
    return spdlog::level::critical;
  case LogLevel::Off:
    return spdlog::level::off;
  }

  return spdlog::level::off;
}

/// Creates a configured component-specific logger.
inline mqss::Result<std::shared_ptr<spdlog::logger>>
makeLogger(std::string_view name, const LoggingConfig &config,
           std::string_view log_file = {}) {
  try {
    std::vector<spdlog::sink_ptr> sinks;

    if (config.enabled) {
      if (config.console_enabled) {
        sinks.push_back(
            std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
      }

      if (config.file_enabled) {
        if (log_file.empty()) {
          return std::unexpected(mqss::Status::configuration(
              "File logging enabled without a log file path"));
        }

        const std::string path(log_file);
        switch (config.file_mode) {
        case LogFileMode::Append:
          sinks.push_back(
              std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, false));
          break;
        case LogFileMode::Truncate:
          sinks.push_back(
              std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true));
          break;
        case LogFileMode::Rotate:
          sinks.push_back(
              std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                  path, config.rotation_size, config.rotation_files));
          break;
        }
      }
    }

    if (sinks.empty()) {
      sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
    }

    auto logger = std::make_shared<spdlog::logger>(std::string(name),
                                                   sinks.begin(), sinks.end());
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
    logger->set_level(config.enabled ? toSpdlogLevel(config.level)
                                     : spdlog::level::off);
    logger->flush_on(spdlog::level::err);

    return logger;
  } catch (const std::exception &error) {
    return std::unexpected(mqss::Status::configuration(
        std::string{"Failed to create logger: "} + error.what()));
  }
}

} // namespace qoffload
