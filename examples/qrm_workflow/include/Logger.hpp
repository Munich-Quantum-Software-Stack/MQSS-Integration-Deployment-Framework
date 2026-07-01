// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

// Logging utilities for the QRM workflow example.
//
// Provides creation of component-specific spdlog loggers.

#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>
#include <vector>

namespace mqss::examples::qrm_workflow {

// Create a logger writing to both the console and a log file.
inline std::shared_ptr<spdlog::logger>
makeLogger(std::string_view name, std::string_view log_file = {}) {
  std::vector<spdlog::sink_ptr> sinks{
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
  };

  if (!log_file.empty()) {
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        std::string(log_file), true));
  }

  auto logger = std::make_shared<spdlog::logger>(std::string(name),
                                                 sinks.begin(), sinks.end());

  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
  logger->set_level(spdlog::level::info);
  logger->flush_on(spdlog::level::info);

  return logger;
}

} // namespace mqss::examples::qrm_workflow
