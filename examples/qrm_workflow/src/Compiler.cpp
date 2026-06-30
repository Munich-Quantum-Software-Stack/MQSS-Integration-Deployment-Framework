// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "Config.hpp"
#include "Logger.hpp"

#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <unordered_map>

std::string stripSpuriousGateDefs(const std::string& qasm) {
    std::istringstream stream(qasm);
    std::ostringstream result;
    std::string line;
    bool inGateBlock = false;
    while (std::getline(stream, line)) {
        if (line.find("gate ") != std::string::npos && 
            line.find("{") != std::string::npos) {
            inGateBlock = true;
            continue;
        }
        if (inGateBlock) {
            if (line.find("}") != std::string::npos) {
                inGateBlock = false;
            }
            continue;
        }
        result << line << "\n";
    }
    return result.str();
}

// Invokes cudaq-quake on `src_path` which convert the source to quake mlir
// dialect. Then mqss-cudaq-opt is called to invoke mqss-passes on the quake
// dialect Finally, cudaq-translate is called to translate the output to qasm or
// qir. Returns the raw Quake MLIR as a string. result_type can be "qir",
// "qir-full", "qir-adaptive", "qir-base", "openqasm2"
static std::string
lowerToOutputFormat(const std::string &src_path, int opt_level,
                    const std::string &target_qpu,
                    const std::string &result_type) {

  // Write output to a temp file
  char tmp_path[] = "/tmp/mqss_quake_XXXXXX";
  int fd = mkstemp(tmp_path);
  if (fd < 0)
    throw std::runtime_error("mkstemp failed");

  close(fd);

  std::string decomposition_cmd;
  if (target_qpu == "iqm")
    decomposition_cmd = "--iqm-gate-set-mapping";
  else if (target_qpu == "fermioniq")
    decomposition_cmd = "--fermioniq-gate-set-mapping";
  else if (target_qpu == "ionq")
    decomposition_cmd = "--ionq-gate-set-mapping";
  else if (target_qpu == "oqc")
    decomposition_cmd = "--oqc-gate-set-mapping";
  else
    decomposition_cmd = "";

  auto tools = mqss::examples::qrm_workflow::getConfig().tools;

  const std::string cmd =
      std::string(tools.cudaq_quake) + " " + src_path + " | " +
      std::string(tools.mqss_cudaq_opt) + " --O" + std::to_string(opt_level) +
      " | " + std::string(tools.cudaq_opt) + " " + decomposition_cmd + " | " +
      std::string(tools.cudaq_translate) + " --convert-to=" + result_type +
      " -o " + tmp_path;

  spdlog::info("Shell command: {}", cmd);

  int ret = std::system(cmd.c_str());
  if (ret != 0) {
    std::remove(tmp_path);
    throw std::runtime_error("MQSS compiler pipeline failed with code: " +
                             std::to_string(ret));
  }

  // Read the qasm/qir output
  FILE *f = std::fopen(tmp_path, "r");
  if (!f) {
    std::remove(tmp_path);
    throw std::runtime_error("failed to open compiler output file");
  }

  std::string result;
  std::array<char, 4096> buf;
  while (std::fgets(buf.data(), buf.size(), f))
    result += buf.data();

  std::fclose(f);
  std::remove(tmp_path);

  return result;
}

static void applyOptimizationPasses(mqss::QuantumTask &task) {
  std::unordered_map<int, std::string> updated_circuit_files;

  auto input_circuit_files = task.circuit_files();

  for (unsigned i = 0; i < input_circuit_files.size(); ++i) {
    const auto &circuit_file = input_circuit_files[i];
    const auto &opt_level = task.optimisation_level();
    const auto &qpu = task.preferred_qpu();

    // Final argument to lowerToOutputFormat can be set to:
    // "qir", "qir-full", "qir-adaptive", "qir-base", "openqasm2"
    std::string result_type = "openqasm2";
    auto out_res = lowerToOutputFormat(circuit_file, opt_level, qpu, result_type);
    if(result_type == "openqasm2"){
      out_res = stripSpuriousGateDefs(out_res);
    }

    updated_circuit_files[i] = out_res;
  }

  for (const auto &[index, new_circuit_file] : updated_circuit_files) {
    task.set_circuit_files(index, new_circuit_file);
  }
}

int main(int argc, char **argv) {
  mqss::examples::qrm_workflow::initConfig(
      mqss::examples::qrm_workflow::loadConfig(argc, argv));

  auto config = mqss::examples::qrm_workflow::getConfig();

  auto logger = mqss::examples::qrm_workflow::makeLogger(
      config.logging.compiler_logger, config.logging.compiler_log);

  // Use a process-specific default logger to avoid passing logger objects.
  spdlog::set_default_logger(logger);

  spdlog::info("Starting MQSS compiler");

  mqss::TransportOptions<mqss::RabbitMqSimple> opts;
  opts.host = std::string(config.rabbitmq.host);
  opts.port = config.rabbitmq.port;
  opts.username = std::string(config.rabbitmq.user);
  opts.password = std::string(config.rabbitmq.password);

  mqss::Messenger<mqss::RabbitMqSimple, mqss::ProtoJson> messenger(opts);

  while (true) {
    spdlog::info("Waiting for a new job");

    auto res = messenger.receive<mqss::QuantumTask>(
        {std::string(config.queues.compiler)},
        mqss::ReceiveArgs{
            .timeout = std::chrono::milliseconds(5000),
            .ack_mode = mqss::AckMode::Auto,
        });

    if (!res.has_value()) {
      // Timeout is normal - just keep polling
      if (res.error().code() == mqss::StatusCode::Timeout) {
        continue;
      }

      spdlog::error("Receive error: {}", res.error().reason());
      continue;
    }

    auto task = *res;

    spdlog::info("Processing task: {}", task.task_id());

    try {
      applyOptimizationPasses(task);
    } catch (const std::exception &e) {
      spdlog::error("Failed to process task {}: {}", task.task_id(), e.what());
      continue;
    }

    spdlog::info("-->Compiler output:");
    for(auto c : task.circuit_files()){
      spdlog::info(c);
    }

   auto send_st = messenger.send<mqss::QuantumTask>(
        {std::string(config.queues.submitter)}, task);

    logger->info("Task sent by compiler!");
    if (!send_st.ok()) {
      spdlog::error("Failed to send result for task {}: {}", task.task_id(),
                    send_st.reason());
      continue;
    }

    spdlog::info("Result sent for task: {}", task.task_id());
  }

  return 0;
}
