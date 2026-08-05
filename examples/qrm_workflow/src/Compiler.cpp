// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copyright (c) MQSS Maintainers

#include "Config.hpp"
#include "Logger.hpp"

#include "Passes/Transforms/Dialects.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mqss/Messenger.hpp"
#include "mqss/Protocol.hpp"
#include "mqss/Transport.hpp"
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include "Passes/Transforms/Transforms.h"
#include "Passes/Transforms/Pipelines.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <unordered_map>

std::string stripSpuriousGateDefs(const std::string &qasm) {
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


// Invokes MQSS Compiler Passes on the input MLIR dialect found via `src_path`.
// Also, converts the optimized/transformed dialect to OpenQasm2 (currently).
static std::string lowerToOutputFormat(const std::string &src_path,
                                       int opt_level,
                                       const std::string &target_qpu,
                                       const std::string &result_type) {

  // Write output to a temp file
  char tmp_path[] = "/tmp/mqss_quake_XXXXXX";
  int fd = mkstemp(tmp_path);
  if (fd < 0)
    throw std::runtime_error("mkstemp failed");

  close(fd);

  // 1. Create a MLIR context
  auto contextptr = mqss::opt::createMQSSContext();
  auto mlirctx = contextptr.get();
  // 2. Parse the input MLIR dialect and create mlir::ModuleOp
  auto module = mlir::parseSourceFile<mlir::ModuleOp>(
      src_path, mlirctx);
  if (!module) {
    spdlog::error("failed to parse MLIR file\n");
  }

  // 3. Declare the Pass Manager.
  mlir::PassManager pm(mlirctx);

  // 4. Register -O1 Optimization/Transformation pass pipeline in the Pass Manager
  mqss::opt::O1(pm);
  // 5. Run the Optimization/Transformation passes
  if (mlir::failed(pm.run(*module))) { 
    spdlog::error("Compiler: Pipeline failed\n");
  }

  // 6. Register and RUN the transpilation passes 
  //   (Currently only BasisConversion or native-gate set decomposition)
  BasisConversionPassOptions options;
  if(target_qpu == "planqc")
    options.gates= "rx,cz,rz";                   // Using PLANQC's native gate-set in this example
  else
   options.gates = "phased_rx, cz";             // IQM's native gate set (Not tested)

  pm.addPass(mqss::opt::createBasisConversionPass(options));

  std::string result;
  llvm::raw_string_ostream resultStream(result);
  // 7. Convert the final MLIR dialect to OpenQASM2
  // Note: PLANQC backend supports OpenQASM2. The MQT-Core DDSIM QDMI device
  //       also supports OpenQASM2. Therefore in the future, the MQT-Core DDSIM
  //       device can be easliy replaced with the PLANQC QDMI device.
  if(result_type == "OpenQasm2"){
    pm.addPass(mqss::opt::QuakeToQASM2Pass(resultStream));
  }
  if (mlir::failed(pm.run(*module))) { 
    spdlog::error("Compiler: Conversion to {} failed", result_type);

  }
  resultStream.flush();
  
  return result;
}

static void applyOptimizationPasses(mqss::QuantumTask &task) {
  std::unordered_map<int, std::string> updated_circuit_files;

  auto input_circuit_files = task.circuit_files();

  for (unsigned i = 0; i < input_circuit_files.size(); ++i) {
    const auto &circuit_file = input_circuit_files[i];
    const auto &opt_level = task.optimisation_level();
    const auto &qpu = task.preferred_qpu();

    // Final argument to lowerToOutputFormat can be set to.
    // Currently: OpenQasm2.
    std::string result_type = "OpenQasm2";
    auto out_res =
        lowerToOutputFormat(circuit_file, opt_level, qpu, result_type);
    if (result_type == "OpenQasm2") {
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
    for (auto c : task.circuit_files()) {
      spdlog::info(c);
    }

    auto send_st = messenger.send<mqss::QuantumTask>(
        {std::string(config.queues.submitter)}, task);

    spdlog::info("Task sent by compiler!");
    if (!send_st.ok()) {
      spdlog::error("Failed to send result for task {}: {}", task.task_id(),
                    send_st.reason());
      continue;
    }

    spdlog::info("Result sent for task: {}", task.task_id());
  }

  return 0;
}
