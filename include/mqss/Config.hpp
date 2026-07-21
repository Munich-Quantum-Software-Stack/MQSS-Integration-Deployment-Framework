// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Generic configuration loading, parsing, and storage.
///
/// A unique tag identifies an independent process-wide configuration instance.
///
/// Different tags provide independent configuration storage even when the
/// configuration type is identical.

#pragma once

#include "mqss/Status.hpp"

#include <charconv>
#include <concepts>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace mqss::config {

/// Returns the value of the first defined environment variable.
inline const char *getEnv(std::initializer_list<const char *> names) {
  for (const char *name : names) {
    if (const char *value = std::getenv(name)) {
      return value;
    }
  }

  return nullptr;
}

/// Returns an environment string or the supplied fallback value.
inline std::string getEnvOr(std::initializer_list<const char *> names,
                            std::string_view fallback) {
  if (const char *value = getEnv(names)) {
    return value;
  }

  return std::string(fallback);
}

/// Parses an integer environment value or returns the supplied fallback.
template <std::integral Integer>
  requires(!std::same_as<Integer, bool>)
Result<Integer> getEnvOr(std::initializer_list<const char *> names,
                         Integer fallback) {
  const char *value = getEnv(names);
  if (value == nullptr) {
    return fallback;
  }

  Integer result{};
  const std::string_view text(value);
  const auto [ptr, error] =
      std::from_chars(text.data(), text.data() + text.size(), result);

  if (error == std::errc{} && ptr == text.data() + text.size()) {
    return result;
  }

  return std::unexpected(Status::configuration(
      std::string{"Invalid integer value for "} + *names.begin()));
}

/// Parses a Boolean environment value or returns the supplied fallback.
inline Result<bool> getEnvOr(std::initializer_list<const char *> names,
                             bool fallback) {
  const char *value = getEnv(names);
  if (value == nullptr) {
    return fallback;
  }

  const std::string_view text(value);
  if (text == "1" || text == "true" || text == "TRUE" || text == "t" ||
      text == "T" || text == "yes" || text == "YES" || text == "on" ||
      text == "ON") {
    return true;
  }

  if (text == "0" || text == "false" || text == "FALSE" || text == "f" ||
      text == "F" || text == "no" || text == "NO" || text == "off" ||
      text == "OFF") {
    return false;
  }

  return std::unexpected(Status::configuration(
      std::string{"Invalid boolean value for "} + *names.begin()));
}

namespace detail {

template <typename Config, typename Tag>
std::optional<Config> &configStorage() {
  static std::optional<Config> config;
  return config;
}

} // namespace detail

/// Loads configuration using defaults, environment, then command-line options.
///
/// The application tag selects the project-specific handlers and storage.
template <typename Config, typename Tag>
Result<Config> loadConfig(Tag tag, int argc, char **argv) {
  Config config = makeDefaultConfig(tag);

  if (auto status = applyEnvironment(tag, config); !status.ok()) {
    return std::unexpected(std::move(status));
  }

  if (auto status = applyCommandLine(tag, config, argc, argv); !status.ok()) {
    return std::unexpected(std::move(status));
  }

  return config;
}

/// Stores the process-wide immutable configuration selected by the tag.
template <typename Config, typename Tag>
Status initConfig(Tag, Config config) {
  auto &stored = detail::configStorage<Config, Tag>();
  if (stored) {
    return Status::configuration("Configuration already initialized");
  }

  stored = std::move(config);
  return Status::success();
}

/// Returns the initialized process-wide configuration selected by the tag.
template <typename Config, typename Tag>
Result<std::reference_wrapper<const Config>> getConfig(Tag) {
  const auto &stored = detail::configStorage<Config, Tag>();
  if (!stored) {
    return std::unexpected(
        Status::configuration("Configuration not initialized"));
  }

  return std::cref(*stored);
}

} // namespace mqss::config
