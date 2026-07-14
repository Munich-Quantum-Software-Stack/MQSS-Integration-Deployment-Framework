// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

#include "mqss/Protocol.hpp"
#include "qoffload/ResourceRegistry.hpp"
#include "qoffload/TaskStore.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace qoffload::protocol {

struct CreateQuantumTaskCommand {
  int shots{};
  std::vector<std::string> circuit_files;
  std::string circuit_format;
  std::string resource_name;
  bool no_modify{};
};

struct GetTaskStatusCommand {
  std::uint64_t task_id{};
};
struct GetTaskResultCommand {
  std::uint64_t task_id{};
};
struct GetCancelReasonCommand {
  std::uint64_t task_id{};
};
struct ListResourcesCommand {};
struct GetResourceInfoCommand {
  std::string resource_name;
};
struct QueryPendingTasksCommand {
  std::string resource_name;
};
struct InvalidControlCommand {
  std::string message;
};

using ControlCommand =
    std::variant<CreateQuantumTaskCommand, GetTaskStatusCommand,
                 GetTaskResultCommand, GetCancelReasonCommand,
                 ListResourcesCommand, GetResourceInfoCommand,
                 QueryPendingTasksCommand, InvalidControlCommand>;

struct TaskCreatedResult {
  std::uint64_t task_id{};
};
struct TaskStatusResult {
  TaskStatus status{TaskStatus::NotFound};
};
struct TaskResultResult {
  CompletedControlTask task;
};
struct CancelReasonResult {
  std::string reason;
};
struct ResourceListResult {
  std::vector<ResourceInfo> resources;
};
struct ResourceInfoResult {
  ResourceInfo resource;
};
struct PendingTasksResult {
  std::size_t count{};
};

/// These protocol-visible messages are for diagnostics only and are not stable
/// error identifiers.
struct ControlError {
  std::string message;
};

using ControlResult =
    std::variant<TaskCreatedResult, TaskStatusResult, TaskResultResult,
                 CancelReasonResult, ResourceListResult, ResourceInfoResult,
                 PendingTasksResult, ControlError>;

/// A command and its reply destination.
struct DecodedControlRequest {
  ControlCommand command;
  std::string response_queue;
};

/// Decoder and encoder for the typed APIRequest/APIResponse Control protocol.
class ControlProtocol {
public:
  DecodedControlRequest decode(const mqss::APIRequest &request) const;

  mqss::APIResponse encode(const ControlResult &result,
                           std::string destination_queue) const;
};

} // namespace qoffload::protocol
