// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// In-memory transport backend.

#pragma once

#include <mqss/transport/Transport.hpp>

#include <memory>

namespace mqss {

template <class TransportTag>
struct TransportOptions;

template <class TransportTag>
std::unique_ptr<Transport>
createTransport(const TransportOptions<TransportTag> &options = {});

/// In-memory transport backend tag.
struct InMemory {};

template <>
struct TransportOptions<InMemory> {};

template <>
std::unique_ptr<Transport>
createTransport<InMemory>(const TransportOptions<InMemory> &options);

} // namespace mqss
