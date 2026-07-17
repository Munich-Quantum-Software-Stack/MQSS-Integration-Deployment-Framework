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

/// Private adapter interface shared by all control protocols.
class ControlProtocol {
public:
  virtual ~ControlProtocol() = default;

  virtual DecodedControlRequest
  decode(const mqss::APIRequest &request) const = 0;
  virtual mqss::APIResponse encode(const ControlResult &result,
                                   std::string destination_queue) const = 0;
};

/// Adapter for the fully typed protobuf control protocol.
class TypedControlProtocol final : public ControlProtocol {
public:
  DecodedControlRequest decode(const mqss::APIRequest &request) const override;
  mqss::APIResponse encode(const ControlResult &result,
                           std::string destination_queue) const override;
};

/// Adapter for the REST-like control protocol.
///
/// Responses are returned in APIResponse.response_body. Unlike the Python
/// implementation, the full APIResponse is serialized instead of only the
/// response body.
class RestControlProtocol final : public ControlProtocol {
public:
  DecodedControlRequest decode(const mqss::APIRequest &request) const override;
  mqss::APIResponse encode(const ControlResult &result,
                           std::string destination_queue) const override;
};

/// Selects the protocol once so the same adapter decodes the request and
/// encodes its response.
inline const ControlProtocol &
controlProtocolFor(const mqss::APIRequest &request) noexcept {
  static const TypedControlProtocol typed_protocol;
  static const RestControlProtocol rest_protocol;

  if (request.typed_request_case() != mqss::APIRequest::TYPED_REQUEST_NOT_SET) {
    return typed_protocol;
  }
  return rest_protocol;
}

} // namespace qoffload::protocol
