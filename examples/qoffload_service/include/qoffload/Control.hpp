// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Public entry point for the QOffload Control interface.
///
/// Protocol decoding, semantic commands, command execution, and response
/// encoding are private implementation details of the Control subsystem.

#include "mqss/Protocol.hpp"

#include <optional>

namespace qoffload {

struct Config;
class Forward;
class ResourceRegistry;
class TaskStore;

/// Result of processing one Control-interface request.
struct ControlMessageOutcome {
  /// Task to forward to QRM, if the request creates a task.
  std::optional<mqss::QuantumTask> outbound_task;

  /// Response to return to the Control client.
  mqss::APIResponse response;
};

/// Implements the QOffload Control interface.
///
/// The current MQSS APIRequest/APIResponse protocol is hidden behind this
/// service and may be replaced without changing its caller-facing API.
class Control {
public:
  /// Decodes, executes, and encodes one Control-interface request.
  ControlMessageOutcome process(const mqss::APIRequest &request,
                                TaskStore &tasks, const Config &config,
                                const ResourceRegistry &resources,
                                Forward &forwarder) const;
};

} // namespace qoffload
