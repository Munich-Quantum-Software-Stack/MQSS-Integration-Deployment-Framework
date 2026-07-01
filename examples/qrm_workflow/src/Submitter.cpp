// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "Config.hpp"
#include "Logger.hpp"

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"

#include "qdmi/client.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <cassert>
#include <csignal>
#include <cstring>
#include <string>
#include <vector>

// Use QDMI API's to create a QDMI Job and submit it to the QDMI device.
// Note: QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM is not set here. Therefore, the
// Driver
//       internally uses its default value i.e. 2. This means that "weak
//       simulation" is selected i.e. sampling from the distribution produced by
//       the circuit. Therefore, Histogram results are queried from the QDMI
//       device. If QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM is set to 0, strong
//       simulation is selected and then full state vector results can be
//       queried.
static mqss::QuantumResult
createAndSubmitQDMIJobToQDMIDevice(mqss::QuantumTask task) {

  QDMI_Job job = nullptr;
  int ret = 0;

  // qdmi_main_driver::Driver &driver = driver_ref.get();
  QDMI_Session session = nullptr;
  // driver.sessionAlloc(&session);
  // session -> enumerate devices (driver has already dlopen'd them, incl.
  // libdevice_x.so)
  QDMI_session_alloc(&session);
  QDMI_session_init(session);

  size_t size = 0;
  QDMI_session_query_session_property(session, QDMI_SESSION_PROPERTY_DEVICES, 0,
                                      nullptr, &size);
  std::vector<QDMI_Device> devices(size / sizeof(QDMI_Device));
  QDMI_session_query_session_property(session, QDMI_SESSION_PROPERTY_DEVICES,
                                      size, devices.data(), nullptr);

  // Three devices are specified currently: MQT_NA, MQT_SC, MQT_DDSIM
  QDMI_Device device = devices.back(); // the vendor device i.e. MQT_DDSIM

  auto circuit = task.circuit_files()[0];
  ret = QDMI_device_create_job(device, &job);
  assert(ret == QDMI_SUCCESS);

  spdlog::info("Created QDMI Job...");
  // Set Properties for the Job
  // Properties set:
  //    1. Circuit format (QIR-base profile or OPENQASM2)
  //    2. Circuit size and source string
  //    3. Number of shots
  const auto format = QDMI_PROGRAM_FORMAT_QASM2;
  ret = QDMI_job_set_parameter(job, QDMI_JOB_PARAMETER_PROGRAMFORMAT,
                               sizeof(QDMI_Program_Format), &format);

  assert(ret == QDMI_SUCCESS);

  int num_shots = task.n_shots();
  ret = QDMI_job_set_parameter(job, QDMI_JOB_PARAMETER_PROGRAM,
                               circuit.size() + 1, circuit.c_str());
  assert(ret == QDMI_SUCCESS);

  if (num_shots > 0) {
    ret = QDMI_job_set_parameter(job, QDMI_JOB_PARAMETER_SHOTSNUM,
                                 sizeof(size_t), &num_shots);
    assert(ret == QDMI_SUCCESS);
    spdlog::info("--> QDMI Num shots: " + std::to_string(num_shots));
  }

  spdlog::info("QDMI Job parameters Set...");
  // Submit the job and wait
  ret = QDMI_job_submit(job);
  assert(ret == QDMI_SUCCESS);

  spdlog::info("QDMI Job submitted to QDMI Device...");
  ret = QDMI_job_wait(job, 0);
  if (ret != QDMI_SUCCESS) {
    spdlog::error("QDMI job wait failed with: {}", ret);
  }

  assert(ret == QDMI_SUCCESS);

  QDMI_Job_Status status{};
  ret = QDMI_job_check(job, &status);

  assert(ret == QDMI_SUCCESS);

  // Teardown (in reverse order)
  size_t result_size = 0;
  ret = QDMI_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS, 0, nullptr,
                             &result_size);
  assert(ret == QDMI_SUCCESS);

  std::string key_list(result_size - 1, '\0');
  ret = QDMI_job_get_results(
      job, QDMI_JOB_RESULT_HIST_KEYS, result_size,
      static_cast<void *>(const_cast<char *>(key_list.data())), nullptr);

  assert(ret == QDMI_STATUS::QDMI_SUCCESS);

  std::vector<std::string> key_vec;
  std::string token;
  std::stringstream ss(key_list);
  while (std::getline(ss, token, ',')) {
    key_vec.emplace_back(token);
  }

  size_t val_size = 0;
  ret = QDMI_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES, 0, nullptr,
                             &val_size);
  assert(ret == QDMI_SUCCESS);

  auto type = val_size / sizeof(size_t);

  std::vector<size_t> counts(type);
  ret = QDMI_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES, val_size,
                             static_cast<void *>(counts.data()), nullptr);
  assert(ret == QDMI_SUCCESS);

  // Create the result object
  mqss::QuantumResult result;
  result.set_task_id(task.task_id());

  result.add_executed_circuits(circuit);

  result.set_execution_status(0);

  auto *res = result.add_results();
  if (key_vec.size() != counts.size()) {
    spdlog::error("Size of Keys Vector not equal to Number of Counts!");
  };

  // Gather the counts within the result object
  for (unsigned i = 0; i < key_vec.size(); ++i) {
    (*res->mutable_counts())[key_vec[i]] = counts[i];
  }

  QDMI_job_free(job);
  QDMI_session_free(session);

  return result;
}

int main(int argc, char **argv) {

  mqss::examples::qrm_workflow::initConfig(
      mqss::examples::qrm_workflow::loadConfig(argc, argv));

  auto config = mqss::examples::qrm_workflow::getConfig();

  auto logger = mqss::examples::qrm_workflow::makeLogger(
      config.logging.submitter_logger, config.logging.submitter_log);

  // Use a process-specific default logger to avoid passing logger objects.
  spdlog::set_default_logger(logger);

  spdlog::info("Starting MQSS Submitter");

  mqss::TransportOptions<mqss::RabbitMqSimple> opts;
  opts.host = std::string(config.rabbitmq.host);
  opts.port = config.rabbitmq.port;
  opts.username = std::string(config.rabbitmq.user);
  opts.password = std::string(config.rabbitmq.password);

  mqss::Messenger<mqss::RabbitMqSimple, mqss::ProtoJson> messenger(opts);

  // Poll for incoming tasks
  while (true) {
    spdlog::info("Waiting for a new job...");
    auto res = messenger.receive<mqss::QuantumTask>(
        {config.queues.submitter},
        mqss::ReceiveArgs{
            .timeout = std::chrono::milliseconds(5000),
            .ack_mode = mqss::AckMode::Auto,
        });

    if (!res.has_value()) {
      // Timeout is normal — just keep polling
      if (res.error().code() == mqss::StatusCode::Timeout) {
        continue;
      }
      spdlog::error("Receive error: ", res.error().reason());
      continue;
    }

    mqss::QuantumTask &task = *res;
    spdlog::info("Processing new task with id: {}", task.task_id());

    // Debugging: Print Circuit files received (qasm or qir)...
    // spdlog::info("-->Decoded task id: {}", task.task_id());
    // spdlog::info("-->New Circuit files dump:\n");
    // auto decoded_circuits = task.circuit_files();
    // for (auto circuit : decoded_circuits) {
    //   spdlog::info(circuit);
    // }

    // prepare QDMI job and submit
    // Set the path to the QDMI Device Shared Object file

    auto circuit_result = createAndSubmitQDMIJobToQDMIDevice(task);

    auto send_st = messenger.send<mqss::QuantumResult>(
        {task.result_destination()}, // use the queue the daemon specified
        circuit_result);

    // After send
    spdlog::info("Result sent by the Submitter for task: {}", task.task_id());
    if (!send_st.ok())
      spdlog::error("Failed to send result: {}", send_st.reason());
  }

  return 0;
}
