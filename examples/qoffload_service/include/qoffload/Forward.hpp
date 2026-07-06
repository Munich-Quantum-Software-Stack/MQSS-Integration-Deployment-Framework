// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Task forwarding and result-processing component.

#include "mqss/Protocol.hpp"

#include <cstdint>

namespace qoffload {

/// Moves quantum tasks and results through QOffload's execution pipeline.
class Forward {
public:
  /// Creates the forwarding component using the QOffload instance UID.
  explicit Forward(std::uint64_t instance_uid) : instance_uid_(instance_uid) {}

  /// Accepts a task submitted through the task interface.
  mqss::QuantumTask forwardDirectTask(const mqss::QuantumTask &) { return {}; }

  /// Accepts a task submitted through the management interface.
  mqss::QuantumTask createControlTask(const mqss::QuantumTask &) { return {}; }

  /// Processes a result received from QRM.
  void processResult(const mqss::QuantumResult &) const {}

private:
  std::uint64_t instance_uid_{};
};

} // namespace qoffload
