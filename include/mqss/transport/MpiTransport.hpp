// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// MPI transport backend.

#pragma once

#include "mqss/transport/Transport.hpp"

#include <mpi.h>

#include <memory>

namespace mqss {

/// MPI transport backend tag.
struct Mpi {};

/// Configuration options for the MPI transport backend.
///
/// Address model:
/// `Address::name` is interpreted as a decimal MPI rank within the configured
/// communicator.
template <>
struct TransportOptions<Mpi> {
  /// MPI communicator used by this transport.
  MPI_Comm communicator = MPI_COMM_WORLD;

  /// MPI message tag used for transport messages.
  int tag = 0;

  /// Initialize MPI from the transport if it is not initialized yet.
  bool initialize_mpi = false;

  /// Required MPI thread support level when initializing MPI.
  int required_thread_level = MPI_THREAD_SERIALIZED;
};

/// Create an MPI transport instance.
template <>
std::unique_ptr<Transport>
createTransport<Mpi>(const TransportOptions<Mpi> &options);

} // namespace mqss
