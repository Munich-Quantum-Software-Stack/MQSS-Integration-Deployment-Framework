// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Protobuf-backed protocol message aliases.

#pragma once

#include "v1/messages.pb.h"

namespace mqss {

// Aliases map generated Protobuf types into the mqss namespace,
// providing a stable and transport-independent public interface.

using QuantumTask = mqss::protocol::v1::QuantumTask;
using QuantumResult = mqss::protocol::v1::QuantumResult;
using QSRegisterEntry = mqss::protocol::v1::QSRegisterEntry;
using QSRegistrationInfo = mqss::protocol::v1::QSRegistrationInfo;
using QHeartBeat = mqss::protocol::v1::QHeartBeat;
using QResourceInfo = mqss::protocol::v1::QResourceInfo;

} // namespace mqss
