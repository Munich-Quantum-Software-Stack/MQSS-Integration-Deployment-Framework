// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Protobuf-backed protocol message aliases.

#pragma once

#include "mqss/protocol/v1/messages.pb.h"

namespace mqss {

// Aliases map generated Protobuf types into the mqss namespace,
// providing a stable and transport-independent public interface.

using QuantumTask = mqss::protocol::v1::QuantumTask;
using QuantumResult = mqss::protocol::v1::QuantumResult;
using QSRegisterEntry = mqss::protocol::v1::QSRegisterEntry;
using QSRegistrationInfo = mqss::protocol::v1::QSRegistrationInfo;
using QHeartBeat = mqss::protocol::v1::QHeartBeat;
using QResourceInfo = mqss::protocol::v1::QResourceInfo;
using CreateTaskRequest = mqss::protocol::v1::CreateTaskRequest;
using TaskRequest = mqss::protocol::v1::TaskRequest;
using ResourceRequest = mqss::protocol::v1::ResourceRequest;
using ListResourcesRequest = mqss::protocol::v1::ListResourcesRequest;
using ApiTaskStatus = mqss::protocol::v1::ApiTaskStatus;
using CreateTaskResponse = mqss::protocol::v1::CreateTaskResponse;
using TaskStatusResponse = mqss::protocol::v1::TaskStatusResponse;
using TaskResultResponse = mqss::protocol::v1::TaskResultResponse;
using CancelReasonResponse = mqss::protocol::v1::CancelReasonResponse;
using ResourcesResponse = mqss::protocol::v1::ResourcesResponse;
using ResourceInfoResponse = mqss::protocol::v1::ResourceInfoResponse;
using PendingTasksResponse = mqss::protocol::v1::PendingTasksResponse;
using ErrorResponse = mqss::protocol::v1::ErrorResponse;
using APIRequest = mqss::protocol::v1::APIRequest;
using APIResponse = mqss::protocol::v1::APIResponse;

} // namespace mqss
