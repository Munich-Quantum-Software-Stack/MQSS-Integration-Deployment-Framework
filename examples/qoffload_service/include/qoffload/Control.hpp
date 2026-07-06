// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Management-interface component for task and resource operations.

#include "mqss/Protocol.hpp"

namespace qoffload {

/// Processes requests received through the QOffload management interface.
class Control {
public:
  Control() = default;

  /// Processes one management-interface request.
  mqss::APIResponse process(const mqss::APIRequest &) const { return {}; }
};

} // namespace qoffload
