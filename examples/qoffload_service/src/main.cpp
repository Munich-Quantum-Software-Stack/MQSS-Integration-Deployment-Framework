// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

/// \file
/// Entry point for the QOffload service.

#include "qoffload/Config.hpp"
#include "qoffload/Logger.hpp"
#include "qoffload/QOffload.hpp"

#include <csignal>
#include <filesystem>
#include <iostream>
#include <utility>

#include <pthread.h>

namespace {

void blockStopSignals(sigset_t &signals) {
  sigemptyset(&signals);
  sigaddset(&signals, SIGINT);
  sigaddset(&signals, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &signals, nullptr);
}

int waitForStopSignal(const sigset_t &signals) {
  int received_signal = 0;
  sigwait(&signals, &received_signal);
  return received_signal;
}

} // namespace

int main(int argc, char **argv) {
  // Block termination signals in all threads so the main thread can handle
  // shutdown synchronously with sigwait().
  sigset_t stop_signals{};
  blockStopSignals(stop_signals);

  auto loaded_config = qoffload::loadConfig(argc, argv);
  if (!loaded_config) {
    std::cerr << loaded_config.error().reason() << '\n';
    return 1;
  }

  std::filesystem::create_directories(loaded_config->paths.runtime_dir);
  std::filesystem::create_directories(loaded_config->paths.log_dir);

  const auto log_file =
      qoffload::logPath(loaded_config->paths.log_dir, "qoffload.log");
  auto logger =
      qoffload::makeLogger("qoffload", loaded_config->logging, log_file);
  if (!logger) {
    std::cerr << logger.error().reason() << '\n';
    return 1;
  }

  if (auto status = qoffload::initConfig(std::move(*loaded_config));
      !status.ok()) {
    (*logger)->error("Failed to initialize configuration: {}", status.reason());
    return 1;
  }

  auto config = qoffload::getConfig();
  if (!config) {
    (*logger)->error("Failed to access configuration: {}",
                     config.error().reason());
    return 1;
  }

  (*logger)->info("QOffload starting");
  (*logger)->debug("Request queue: {}", config->get().queues.request);
  (*logger)->debug("Control request queue: {}",
                   config->get().queues.control_request);
  (*logger)->debug("Result queue: {}", config->get().queues.result);
  (*logger)->debug("QRM task queue: {}", config->get().queues.qrm_task);
  (*logger)->debug("Instance UID: {}", config->get().identity.instance_uid);

  qoffload::QOffload qoffload{config->get(), *logger};
  qoffload.start();

  // Keep the process alive until an explicit termination signal is received.
  const auto signal = waitForStopSignal(stop_signals);
  (*logger)->info("QOffload stopping after signal {}", signal);
  qoffload.requestStop();
  return 0;
}
