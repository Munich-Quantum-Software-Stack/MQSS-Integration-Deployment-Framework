// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#pragma once

/// \file
/// Static resource catalog used by Control-interface resource queries.

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace qoffload {

/// Public resource information exposed through the Control interface.
struct ResourceInfo {
  std::string name;
  std::uint32_t qubits{};
  bool online{};
};

/// Read-only registry of known quantum resources.
///
/// The current implementation uses a static table matching the Python
/// reference. It can later be replaced by live resource discovery without
/// changing control request handling.
class ResourceRegistry {
public:
  ResourceRegistry()
      : resources_{
            ResourceInfo{.name = "QLM", .qubits = 38, .online = false},
            ResourceInfo{.name = "Q5", .qubits = 5, .online = false},
            ResourceInfo{.name = "Q20", .qubits = 20, .online = true},
            ResourceInfo{.name = "AQT20", .qubits = 20, .online = true},
            ResourceInfo{.name = "QExa20", .qubits = 20, .online = true},
        } {}

  const std::vector<ResourceInfo> &resources() const noexcept {
    return resources_;
  }

  /// Returns resource information for `name`, or std::nullopt if unknown.
  std::optional<ResourceInfo> find(std::string_view name) const {
    const auto it =
        std::ranges::find_if(resources_, [name](const ResourceInfo &resource) {
          return resource.name == name;
        });

    if (it == resources_.end()) {
      return std::nullopt;
    }

    return *it;
  }

private:
  /// Static resource catalog exposed through Control queries.
  std::vector<ResourceInfo> resources_;
};

} // namespace qoffload
