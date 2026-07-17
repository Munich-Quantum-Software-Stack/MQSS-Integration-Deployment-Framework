// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "ControlProtocol.hpp"

#include <charconv>
#include <cmath>
#include <google/protobuf/struct.pb.h>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace qoffload::protocol {

namespace {

constexpr std::string_view GetMethod = "GET";
constexpr std::string_view PostMethod = "POST";
constexpr std::string_view JobPath = "job";
constexpr std::string_view ResourcesPath = "resources";
constexpr std::string_view StatusPath = "status";
constexpr std::string_view ResultPath = "result";
constexpr std::string_view CancelReasonPath = "cancel_reason";
constexpr std::string_view PendingJobsPath = "num_pending_jobs";

constexpr std::string_view InvalidMethod = "INVALID METHOD";
constexpr std::string_view InvalidJobRequest = "INVALID JOB REQUEST";
constexpr std::string_view InvalidJobId = "INVALID JOB ID";
constexpr std::string_view JobNotFound = "JOB NOT FOUND";
constexpr std::string_view UnknownControlRequest = "UNKNOWN CONTROL REQUEST";

ControlCommand invalid(std::string_view message) {
  return InvalidControlCommand{.message = std::string{message}};
}

std::vector<std::string_view> splitPath(std::string_view path) {
  while (path.starts_with('/')) {
    path.remove_prefix(1);
  }
  while (path.ends_with('/') && !path.empty()) {
    path.remove_suffix(1);
  }

  std::vector<std::string_view> parts;
  while (!path.empty()) {
    const auto separator = path.find('/');
    parts.push_back(path.substr(0, separator));
    if (separator == std::string_view::npos) {
      break;
    }
    path.remove_prefix(separator + 1);
  }
  return parts;
}

std::optional<std::uint64_t> parseTaskId(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }
  std::uint64_t value{};
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

const google::protobuf::Value *findField(const google::protobuf::Struct &data,
                                         std::string_view key) {
  const auto field = data.fields().find(std::string{key});
  return field == data.fields().end() ? nullptr : &field->second;
}

std::optional<int> requiredInt(const google::protobuf::Struct &data,
                               std::string_view key) {
  const auto *value = findField(data, key);
  if (value == nullptr ||
      value->kind_case() != google::protobuf::Value::kNumberValue) {
    return std::nullopt;
  }

  const auto number = value->number_value();
  if (!std::isfinite(number) || std::trunc(number) != number ||
      number < std::numeric_limits<int>::min() ||
      number > std::numeric_limits<int>::max()) {
    return std::nullopt;
  }
  return static_cast<int>(number);
}

std::optional<bool> requiredBool(const google::protobuf::Struct &data,
                                 std::string_view key) {
  const auto *value = findField(data, key);
  if (value == nullptr ||
      value->kind_case() != google::protobuf::Value::kBoolValue) {
    return std::nullopt;
  }
  return value->bool_value();
}

std::optional<std::string> requiredString(const google::protobuf::Struct &data,
                                          std::string_view key) {
  const auto *value = findField(data, key);
  if (value == nullptr ||
      value->kind_case() != google::protobuf::Value::kStringValue) {
    return std::nullopt;
  }
  return value->string_value();
}

std::optional<std::vector<std::string>>
requiredStringList(const google::protobuf::Struct &data, std::string_view key) {
  const auto *value = findField(data, key);
  if (value == nullptr ||
      value->kind_case() != google::protobuf::Value::kListValue) {
    return std::nullopt;
  }

  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(value->list_value().values_size()));
  for (const auto &entry : value->list_value().values()) {
    if (entry.kind_case() != google::protobuf::Value::kStringValue) {
      return std::nullopt;
    }
    values.push_back(entry.string_value());
  }
  return values;
}

ControlCommand createTaskCommand(const mqss::APIRequest &request) {
  if (request.method() != PostMethod) {
    return invalid(InvalidMethod);
  }

  const auto shots = requiredInt(request.data(), "shots");
  const auto circuit = requiredStringList(request.data(), "circuit");
  const auto circuit_format = requiredString(request.data(), "circuit_format");
  const auto resource_name = requiredString(request.data(), "resource_name");
  const auto no_modify = requiredBool(request.data(), "no_modify");

  if (!shots || *shots < 0 || !circuit || !circuit_format || !resource_name ||
      !no_modify) {
    return invalid(InvalidJobRequest);
  }

  return CreateQuantumTaskCommand{
      .shots = *shots,
      .circuit_files = std::move(*circuit),
      .circuit_format = std::move(*circuit_format),
      .resource_name = std::move(*resource_name),
      .no_modify = *no_modify,
  };
}

void setString(google::protobuf::Struct &body, std::string_view key,
               std::string value) {
  (*body.mutable_fields())[std::string{key}].set_string_value(std::move(value));
}

void setNumber(google::protobuf::Struct &body, std::string_view key,
               double value) {
  (*body.mutable_fields())[std::string{key}].set_number_value(value);
}

void setBool(google::protobuf::Struct &body, std::string_view key, bool value) {
  (*body.mutable_fields())[std::string{key}].set_bool_value(value);
}

void setError(google::protobuf::Struct &body, std::string message) {
  setString(body, "Error", std::move(message));
}

void setTaskResult(google::protobuf::Struct &body,
                   const CompletedControlTask &task) {
  auto *results = (*body.mutable_fields())["result"].mutable_list_value();
  for (const auto &counts : task.result_counts) {
    auto *entry = results->add_values()->mutable_struct_value();
    for (const auto &[state, count] : counts) {
      (*entry->mutable_fields())[state].set_number_value(count);
    }
  }
  setString(body, "timestamp_scheduled", task.timestamp_scheduled);
  setString(body, "timestamp_submitted", task.timestamp_submitted);
  setString(body, "timestamp_completed", task.timestamp_completed);
}

void setResource(google::protobuf::Struct &body, const ResourceInfo &resource) {
  setString(body, "name", resource.name);
  setNumber(body, "qubits", resource.qubits);
  setBool(body, "online", resource.online);
}

void setResources(google::protobuf::Struct &body,
                  const std::vector<ResourceInfo> &resources) {
  auto *list = (*body.mutable_fields())["resources"].mutable_list_value();
  for (const auto &resource : resources) {
    auto *entry = list->add_values()->mutable_struct_value();
    setResource(*entry, resource);
  }
}

} // namespace

DecodedControlRequest
RestControlProtocol::decode(const mqss::APIRequest &request) const {
  DecodedControlRequest decoded{
      .command = invalid(UnknownControlRequest),
      .response_queue = request.response_queue(),
  };

  const auto path = splitPath(request.request());
  if (path.size() == 1 && path[0] == JobPath) {
    decoded.command = createTaskCommand(request);
    return decoded;
  }

  if (request.method() != GetMethod) {
    decoded.command = invalid(InvalidMethod);
    return decoded;
  }

  if (path.size() == 3 && path[0] == JobPath) {
    const auto task_id = parseTaskId(path[1]);
    if (!task_id) {
      decoded.command = invalid(InvalidJobId);
    } else if (path[2] == StatusPath) {
      decoded.command = GetTaskStatusCommand{.task_id = *task_id};
    } else if (path[2] == ResultPath) {
      decoded.command = GetTaskResultCommand{.task_id = *task_id};
    } else if (path[2] == CancelReasonPath) {
      decoded.command = GetCancelReasonCommand{.task_id = *task_id};
    }
    return decoded;
  }

  if (path.size() == 1 && path[0] == ResourcesPath) {
    decoded.command = ListResourcesCommand{};
  } else if (path.size() == 2 && path[0] == ResourcesPath && !path[1].empty()) {
    decoded.command =
        GetResourceInfoCommand{.resource_name = std::string{path[1]}};
  } else if (path.size() == 3 && path[0] == ResourcesPath && !path[1].empty() &&
             path[2] == PendingJobsPath) {
    decoded.command =
        QueryPendingTasksCommand{.resource_name = std::string{path[1]}};
  }

  return decoded;
}

mqss::APIResponse
RestControlProtocol::encode(const ControlResult &result,
                            std::string destination_queue) const {
  mqss::APIResponse response;
  response.set_destination_queue(std::move(destination_queue));
  auto &body = *response.mutable_response_body();

  std::visit(
      [&](const auto &value) {
        using Result = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<Result, TaskCreatedResult>) {
          setString(body, "uuid", std::to_string(value.task_id));
        } else if constexpr (std::is_same_v<Result, TaskStatusResult>) {
          switch (value.status) {
          case TaskStatus::Waiting:
            setString(body, "status", "WAITING");
            break;
          case TaskStatus::Completed:
            setString(body, "status", "COMPLETED");
            break;
          case TaskStatus::Cancelled:
            setString(body, "status", "CANCELLED");
            break;
          case TaskStatus::NotFound:
            setError(body, std::string{JobNotFound});
            break;
          }
        } else if constexpr (std::is_same_v<Result, TaskResultResult>) {
          setTaskResult(body, value.task);
        } else if constexpr (std::is_same_v<Result, CancelReasonResult>) {
          setString(body, "cancel_reason", value.reason);
        } else if constexpr (std::is_same_v<Result, ResourceListResult>) {
          setResources(body, value.resources);
        } else if constexpr (std::is_same_v<Result, ResourceInfoResult>) {
          setResource(body, value.resource);
        } else if constexpr (std::is_same_v<Result, PendingTasksResult>) {
          setNumber(body, "num_pending_jobs", value.count);
        } else if constexpr (std::is_same_v<Result, ControlError>) {
          setError(body, value.message);
        }
      },
      result);

  return response;
}

} // namespace qoffload::protocol
