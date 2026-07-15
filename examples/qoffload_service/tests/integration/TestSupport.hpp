// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

#include "mqss/Messenger.hpp"
#include "mqss/Transport.hpp"

#include <chrono>
#include <cstdlib>
#include <string>

namespace qoffload::test {

using Messenger = mqss::Messenger<mqss::RabbitMqSimple, mqss::ProtoJson>;

inline std::string getEnvOr(const char *name, std::string fallback) {
  if (const char *value = std::getenv(name);
      value != nullptr && *value != '\0') {
    return value;
  }
  return fallback;
}

inline int getEnvIntOr(const char *name, int fallback) {
  const auto value = getEnvOr(name, {});
  if (value.empty()) {
    return fallback;
  }

  try {
    return std::stoi(value);
  } catch (...) {
    return fallback;
  }
}

inline mqss::TransportOptions<mqss::RabbitMqSimple> transportOptions() {
  mqss::TransportOptions<mqss::RabbitMqSimple> options;
  options.host = getEnvOr("QOFFLOAD_AMQP_HOST", options.host);
  options.port = getEnvIntOr("QOFFLOAD_AMQP_PORT", options.port);
  options.username = getEnvOr("QOFFLOAD_AMQP_USER", options.username);
  options.password = getEnvOr("QOFFLOAD_AMQP_PASSWORD", options.password);
  options.vhost = getEnvOr("QOFFLOAD_AMQP_VHOST", options.vhost);
  return options;
}

inline constexpr auto receiveTimeout = std::chrono::seconds{30};

} // namespace qoffload::test
