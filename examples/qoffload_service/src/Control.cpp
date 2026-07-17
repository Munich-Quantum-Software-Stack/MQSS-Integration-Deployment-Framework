// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "qoffload/Control.hpp"

#include "control/ControlProtocol.hpp"
#include "qoffload/Config.hpp"
#include "qoffload/Forward.hpp"
#include "qoffload/ResourceRegistry.hpp"
#include "qoffload/TaskStore.hpp"

#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace qoffload {
namespace {

constexpr std::string_view JobIsWaiting = "JOB IS WAITING";
constexpr std::string_view JobNotFound = "JOB NOT FOUND";
constexpr std::string_view ResourceNotFound = "RESOURCE NOT FOUND";

protocol::ControlResult
handleCommand(const protocol::GetTaskStatusCommand &command, TaskStore &tasks,
              const ResourceRegistry &) {
  return protocol::TaskStatusResult{.status = tasks.status(command.task_id)};
}

protocol::ControlResult
handleCommand(const protocol::GetTaskResultCommand &command, TaskStore &tasks,
              const ResourceRegistry &) {
  if (tasks.status(command.task_id) == TaskStatus::Waiting) {
    return protocol::ControlError{.message = std::string{JobIsWaiting}};
  }

  auto result = tasks.takeCompletedControlTask(command.task_id);
  if (!result.has_value()) {
    return protocol::ControlError{.message = std::string{JobNotFound}};
  }

  return protocol::TaskResultResult{.task = std::move(*result)};
}

protocol::ControlResult
handleCommand(const protocol::GetCancelReasonCommand &command, TaskStore &tasks,
              const ResourceRegistry &) {
  if (tasks.status(command.task_id) == TaskStatus::Waiting) {
    return protocol::ControlError{.message = std::string{JobIsWaiting}};
  }

  auto result = tasks.takeCompletedControlTask(command.task_id);
  if (!result.has_value()) {
    return protocol::ControlError{.message = std::string{JobNotFound}};
  }

  return protocol::CancelReasonResult{.reason =
                                          std::move(result->cancel_reason)};
}

protocol::ControlResult handleCommand(const protocol::ListResourcesCommand &,
                                      TaskStore &,
                                      const ResourceRegistry &resources) {
  return protocol::ResourceListResult{.resources = resources.resources()};
}

protocol::ControlResult
handleCommand(const protocol::GetResourceInfoCommand &command, TaskStore &,
              const ResourceRegistry &resources) {
  auto resource = resources.find(command.resource_name);
  if (!resource.has_value()) {
    return protocol::ControlError{.message = std::string{ResourceNotFound}};
  }
  return protocol::ResourceInfoResult{.resource = std::move(*resource)};
}

protocol::ControlResult
handleCommand(const protocol::QueryPendingTasksCommand &command,
              TaskStore &tasks, const ResourceRegistry &resources) {
  if (!resources.find(command.resource_name).has_value()) {
    return protocol::ControlError{.message = std::string{ResourceNotFound}};
  }
  return protocol::PendingTasksResult{.count = tasks.pendingCount()};
}

protocol::ControlResult
handleCommand(const protocol::InvalidControlCommand &command, TaskStore &,
              const ResourceRegistry &) {
  return protocol::ControlError{.message = command.message};
}

struct ControlExecutionOutcome {
  protocol::ControlResult result;
  std::optional<mqss::QuantumTask> outbound_task;
};

void applyCreateTaskCommand(mqss::QuantumTask &task,
                            const protocol::CreateQuantumTaskCommand &command) {
  task.set_n_qbits(0);
  task.set_n_shots(command.shots);
  task.set_circuit_file_type(command.circuit_format);
  task.set_preferred_qpu(command.resource_name);
  task.set_no_modify(command.no_modify);
  task.set_via_hpc(true);
  for (const auto &file : command.circuit_files) {
    task.add_circuit_files(file);
  }
}

ControlExecutionOutcome
executeControlCommand(const protocol::ControlCommand &command, TaskStore &tasks,
                      const Config &config, const ResourceRegistry &resources,
                      Forward &forwarder) {
  return std::visit(
      [&](const auto &typed_command) -> ControlExecutionOutcome {
        using Command = std::remove_cvref_t<decltype(typed_command)>;
        if constexpr (std::is_same_v<Command,
                                     protocol::CreateQuantumTaskCommand>) {
          mqss::QuantumTask task;
          applyCreateTaskCommand(task, typed_command);
          auto forwarded =
              forwarder.createControlTask(std::move(task), tasks, config);
          return {
              .result =
                  protocol::TaskCreatedResult{
                      .task_id =
                          static_cast<std::uint64_t>(forwarded.task_id())},
              .outbound_task = std::move(forwarded),
          };
        } else {
          return {
              .result = handleCommand(typed_command, tasks, resources),
              .outbound_task = std::nullopt,
          };
        }
      },
      command);
}

} // namespace

ControlMessageOutcome Control::process(const mqss::APIRequest &request,
                                       TaskStore &tasks, const Config &config,
                                       const ResourceRegistry &resources,
                                       Forward &forwarder) const {
  const auto &protocol = protocol::controlProtocolFor(request);
  auto decoded = protocol.decode(request);
  auto outcome = executeControlCommand(decoded.command, tasks, config,
                                       resources, forwarder);
  auto response =
      protocol.encode(outcome.result, std::move(decoded.response_queue));

  return {
      .outbound_task = std::move(outcome.outbound_task),
      .response = std::move(response),
  };
}

} // namespace qoffload
