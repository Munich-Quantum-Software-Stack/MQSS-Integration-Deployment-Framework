// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// RabbitMQ transport backend based on SimpleAmqpClient.
///
/// This defines the RabbitMQ-based implementation of the Transport
/// interface. It provides a backend that maps transport addresses to RabbitMQ
/// queues and uses the default exchange for message routing.

#include "mqss/transport/Transport.hpp"

#include <memory>
#include <string>

namespace mqss {

/// RabbitMQ transport backend tag.
struct RabbitMqSimple {};

/// Configuration options for the RabbitMQ transport backend.
/// Default values are suitable for a local broker instance.
template <>
struct TransportOptions<RabbitMqSimple> {
  /// Broker hostname or IP address.
  std::string host = "127.0.0.1";

  /// Broker port.
  int port = 5672;

  /// Username for authentication.
  std::string username = "guest";

  /// Password for authentication.
  std::string password = "guest";

  /// Virtual host.
  std::string vhost = "/";
};

/// Create a RabbitMQ transport instance.
template <>
std::unique_ptr<Transport> createTransport<RabbitMqSimple>(
    const TransportOptions<RabbitMqSimple> &options);

} // namespace mqss
