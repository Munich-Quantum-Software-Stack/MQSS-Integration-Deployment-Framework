// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// In-memory transport backend.

#pragma once

#include "mqss/transport/Transport.hpp"

#include <memory>

namespace mqss {

/// In-memory transport backend tag.
struct InMemory {};

/// In-memory backend doesn't support any options.
template <>
struct TransportOptions<InMemory> {};

/// Create a RabbitMQ transport instance.
template <>
std::unique_ptr<Transport>
createTransport<InMemory>(const TransportOptions<InMemory> &options);

} // namespace mqss
