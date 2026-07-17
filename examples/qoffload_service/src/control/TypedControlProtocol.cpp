// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "ControlProtocol.hpp"

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace qoffload::protocol {

namespace {

constexpr std::string_view InvalidControlRequest = "INVALID CONTROL REQUEST";
constexpr std::string_view JobNotFound = "JOB NOT FOUND";

ControlCommand invalid(std::string_view message) {
  return InvalidControlCommand{.message = std::string{message}};
}

CreateQuantumTaskCommand
makeCreateCommand(const mqss::CreateTaskRequest &request) {
  CreateQuantumTaskCommand command;
  command.shots = request.shots();
  command.circuit_files.assign(request.circuit().begin(),
                               request.circuit().end());
  command.circuit_format = request.circuit_format();
  command.resource_name = request.resource_name();
  command.no_modify = request.no_modify();
  return command;
}

void setResource(mqss::ResourceInfoResponse &response,
                 const ResourceInfo &resource) {
  response.set_name(resource.name);
  response.set_num_qubits(static_cast<std::int32_t>(resource.qubits));
  response.set_online(resource.online);
}

void setTaskResult(mqss::TaskResultResponse &response,
                   const CompletedControlTask &task) {
  for (const auto &counts : task.result_counts) {
    auto *circuit = response.add_result();
    auto *output = circuit->mutable_counts();
    for (const auto &[state, count] : counts) {
      (*output)[state] = count;
    }
  }
  response.set_timestamp_scheduled(task.timestamp_scheduled);
  response.set_timestamp_submitted(task.timestamp_submitted);
  response.set_timestamp_completed(task.timestamp_completed);
}

} // namespace

DecodedControlRequest
TypedControlProtocol::decode(const mqss::APIRequest &request) const {
  DecodedControlRequest decoded{
      .command = invalid(InvalidControlRequest),
      .response_queue = request.response_queue(),
  };

  // The typed protocol identifies the operation only through the selected oneof
  // case. REST-related fields are ignored here.
  switch (request.typed_request_case()) {
  case mqss::APIRequest::kCreateTask:
    decoded.command = makeCreateCommand(request.create_task());
    break;
  case mqss::APIRequest::kTaskStatus:
    decoded.command = GetTaskStatusCommand{
        .task_id = static_cast<std::uint64_t>(request.task_status().uuid())};
    break;
  case mqss::APIRequest::kTaskResult:
    decoded.command = GetTaskResultCommand{
        .task_id = static_cast<std::uint64_t>(request.task_result().uuid())};
    break;
  case mqss::APIRequest::kCancelReason:
    decoded.command = GetCancelReasonCommand{
        .task_id = static_cast<std::uint64_t>(request.cancel_reason().uuid())};
    break;
  case mqss::APIRequest::kListResources:
    decoded.command = ListResourcesCommand{};
    break;
  case mqss::APIRequest::kResourceInfo:
    decoded.command = GetResourceInfoCommand{
        .resource_name = request.resource_info().resource_name()};
    break;
  case mqss::APIRequest::kPendingTasks:
    decoded.command = QueryPendingTasksCommand{
        .resource_name = request.pending_tasks().resource_name()};
    break;
  case mqss::APIRequest::TYPED_REQUEST_NOT_SET:
    break;
  }

  return decoded;
}

mqss::APIResponse
TypedControlProtocol::encode(const ControlResult &result,
                             std::string destination_queue) const {
  mqss::APIResponse response;
  response.set_destination_queue(std::move(destination_queue));

  std::visit(
      [&](const auto &value) {
        using Result = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<Result, TaskCreatedResult>) {
          response.mutable_create_task()->set_uuid(
              static_cast<std::int32_t>(value.task_id));
        } else if constexpr (std::is_same_v<Result, TaskStatusResult>) {
          auto *status = response.mutable_task_status();
          switch (value.status) {
          case TaskStatus::Waiting:
            status->set_status(mqss::ApiTaskStatus::API_TASK_STATUS_WAITING);
            break;
          case TaskStatus::Completed:
            status->set_status(mqss::ApiTaskStatus::API_TASK_STATUS_COMPLETED);
            break;
          case TaskStatus::Cancelled:
            status->set_status(mqss::ApiTaskStatus::API_TASK_STATUS_CANCELLED);
            break;
          case TaskStatus::NotFound:
            response.mutable_error()->set_message(std::string{JobNotFound});
            break;
          }
        } else if constexpr (std::is_same_v<Result, TaskResultResult>) {
          setTaskResult(*response.mutable_task_result(), value.task);
        } else if constexpr (std::is_same_v<Result, CancelReasonResult>) {
          response.mutable_cancel_reason()->set_cancel_reason(value.reason);
        } else if constexpr (std::is_same_v<Result, ResourceListResult>) {
          auto *resources = response.mutable_resources();
          for (const auto &resource : value.resources) {
            resources->add_resources(resource.name);
          }
        } else if constexpr (std::is_same_v<Result, ResourceInfoResult>) {
          setResource(*response.mutable_resource_info(), value.resource);
        } else if constexpr (std::is_same_v<Result, PendingTasksResult>) {
          response.mutable_pending_tasks()->set_num_pending_jobs(
              static_cast<std::int32_t>(value.count));
        } else if constexpr (std::is_same_v<Result, ControlError>) {
          response.mutable_error()->set_message(value.message);
        }
      },
      result);

  return response;
}

} // namespace qoffload::protocol
