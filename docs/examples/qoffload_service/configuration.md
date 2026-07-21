# QOffload Configuration

## Overview

QOffload uses the common MQSS configuration library for loading, validating, and storing application configuration.
QOffload defines the application-specific configuration model, build-time defaults, environment-variable mappings, and command-line hook, while the common library manages the configuration lifecycle.

Configuration is applied in this order:

1. Build-time defaults
2. Environment variables
3. Command-line options

A value supplied by a later source overrides the value from an earlier source.
Environment values are validated and converted to the corresponding C++ types during loading.
After loading, the configuration is initialized as an immutable process-wide object.

The current implementation does not apply command-line overrides. Therefore, QOffload configuration is determined entirely by build-time defaults and environment variables.

## Configuration Files

- `Config.cmake`  
  Defines path defaults that depend on the build-system context.
  The values are exposed as CMake cache variables, so they can be changed when configuring the build.
  CMake substitutes these values into `ConfigDefaults.hpp.in` to produce the generated defaults header.

- `ConfigDefaults.hpp.in`  
  Template for the generated C++ defaults header.
  It contains the complete set of QOffload build-time defaults, including RabbitMQ connections, queue names, identity, policy, logging, and runtime paths.
  Most defaults are fixed constants in the template.
  The runtime, log, and state-file paths contain CMake placeholders populated from `Config.cmake` during configuration.

- `include/qoffload/Config.hpp`  
  Defines the application-facing configuration model and integrates it with the common configuration library.
  The module defines typed configuration structures for all application settings, constructs the initial configuration from generated defaults, applies environment-variable and command-line overrides (currently unused).
  It initializes the process-wide configuration and provides read-only access to it.

## Build-Time Defaults

The following table lists the effective defaults before environment-variable overrides are applied.

| Configuration field       | C++ type        | Default value                                | Defined by              | Description                                               |
| ------------------------- | --------------- | -------------------------------------------: | ----------------------- | --------------------------------------------------------- |
| `rabbitmq.host`           | `std::string`   | `localhost`                                  | `ConfigDefaults.hpp.in` | Client-facing broker host.                                |
| `rabbitmq.port`           | `int`           | `5672`                                       | `ConfigDefaults.hpp.in` | Client-facing broker port.                                |
| `rabbitmq.user`           | `std::string`   | `guest`                                      | `ConfigDefaults.hpp.in` | Client-facing broker username.                            |
| `rabbitmq.password`       | `std::string`   | `guest`                                      | `ConfigDefaults.hpp.in` | Client-facing broker password.                            |
| `rabbitmq.vhost`          | `std::string`   | `/`                                          | `ConfigDefaults.hpp.in` | Client-facing broker virtual host.                        |
| `qrm_rabbitmq.host`       | `std::string`   | `127.0.0.1`                                  | `ConfigDefaults.hpp.in` | QRM broker host.                                          |
| `qrm_rabbitmq.port`       | `int`           | `5672`                                       | `ConfigDefaults.hpp.in` | QRM broker port.                                          |
| `qrm_rabbitmq.user`       | `std::string`   | `guest`                                      | `ConfigDefaults.hpp.in` | QRM broker username.                                      |
| `qrm_rabbitmq.password`   | `std::string`   | `guest`                                      | `ConfigDefaults.hpp.in` | QRM broker password.                                      |
| `qrm_rabbitmq.vhost`      | `std::string`   | `/`                                          | `ConfigDefaults.hpp.in` | QRM broker virtual host.                                  |
| `queues.request`          | `std::string`   | `qoffload_request_reception_queue`           | `ConfigDefaults.hpp.in` | Queue for task submission requests.                       |
| `queues.control_request`  | `std::string`   | `qoffload_api_request_reception_queue`       | `ConfigDefaults.hpp.in` | Queue for control API requests.                           |
| `queues.result`           | `std::string`   | `qoffload_result_reception_queue`            | `ConfigDefaults.hpp.in` | Queue for completed task results.                         |
| `queues.qrm_task`         | `std::string`   | `quantum_task_collection_queue`              | `ConfigDefaults.hpp.in` | Queue for QRM task submissions.                           |
| `identity.instance_uid`   | `std::uint64_t` | `0`                                          | `ConfigDefaults.hpp.in` | Used to generate local task identifiers.                  |
| `identity.user_identity`  | `std::string`   | `qoffload_user`                              | `ConfigDefaults.hpp.in` | QRM task owner.                                           |
| `logging.enabled`         | `bool`          | `true`                                       | `ConfigDefaults.hpp.in` | Enables the logging subsystem.                            |
| `logging.console_enabled` | `bool`          | `true`                                       | `ConfigDefaults.hpp.in` | Writes log messages to the console.                       |
| `logging.file_enabled`    | `bool`          | `false`                                      | `ConfigDefaults.hpp.in` | Writes log messages to disk files.                        |
| `logging.level`           | `LogLevel`      | `info`                                       | `ConfigDefaults.hpp.in` | Log verbosity level.                                      |
| `logging.file_mode`       | `LogFileMode`   | `rotate`                                     | `ConfigDefaults.hpp.in` | File writing strategy.                                    |
| `logging.rotation_size`   | `std::size_t`   | `10,485,760` bytes (10 MiB)                  | `ConfigDefaults.hpp.in` | Maximum log file size before rotation.                    |
| `logging.rotation_files`  | `std::size_t`   | `3`                                          | `ConfigDefaults.hpp.in` | Maximum number of rotated log files.                      |
| `paths.runtime_dir`       | `std::string`   | `${CMAKE_CURRENT_BINARY_DIR}/runtime`        | `Config.cmake`          | Directory for runtime-generated files.                    |
| `paths.log_dir`           | `std::string`   | `${QOFFLOAD_RUNTIME_DIR}/logs`               | `Config.cmake`          | Directory for stored log files.                           |
| `paths.state_file`        | `std::string`   | `${QOFFLOAD_RUNTIME_DIR}/qoffload-state.bin` | `Config.cmake`          | Path to persistent application state file.                |
| `policy.hpc_node`         | `bool`          | `false`                                      | `ConfigDefaults.hpp.in` | Indicates the access path: HPC (`true`) or MQP (`false`). |
| `policy.resume_state`     | `bool`          | `false`                                      | `ConfigDefaults.hpp.in` | Restores persisted application state on startup.          |

The path values defined in `Config.cmake` are CMake cache entries, so their effective defaults depend on the configured build directory and any cache values provided during CMake configuration.

## Environment Variables

Environment variables override the build-time defaults.
A failed conversion of a numeric or Boolean value causes configuration loading to fail.

| Environment variable             | Configuration field       | Value type / accepted values                                               | Default                                | Description                                                      |
| -------------------------------- | ------------------------- | -------------------------------------------------------------------------- | -------------------------------------- | ---------------------------------------------------------------- |
| `QOFFLOAD_AMQP_HOST`             | `rabbitmq.host`           | String                                                                     | `localhost`                            | Client-facing broker host.                                       |
| `QOFFLOAD_AMQP_PORT`             | `rabbitmq.port`           | Integer                                                                    | `5672`                                 | Client-facing broker port.                                       |
| `QOFFLOAD_AMQP_USER`             | `rabbitmq.user`           | String                                                                     | `guest`                                | Client-facing broker username.                                   |
| `QOFFLOAD_AMQP_PASSWORD`         | `rabbitmq.password`       | String                                                                     | `guest`                                | Client-facing broker password.                                   |
| `QOFFLOAD_AMQP_VHOST`            | `rabbitmq.vhost`          | String                                                                     | `/`                                    | Client-facing broker virtual host.                               |
| `QRM_AMQP_HOST`                  | `qrm_rabbitmq.host`       | String                                                                     | `127.0.0.1`                            | QRM broker host.                                                 |
| `QRM_AMQP_PORT`                  | `qrm_rabbitmq.port`       | Integer                                                                    | `5672`                                 | QRM broker port.                                                 |
| `QRM_AMQP_USER`                  | `qrm_rabbitmq.user`       | String                                                                     | `guest`                                | QRM broker username.                                             |
| `QRM_AMQP_PASSWORD`              | `qrm_rabbitmq.password`   | String                                                                     | `guest`                                | QRM broker password.                                             |
| `QRM_AMQP_VHOST`                 | `qrm_rabbitmq.vhost`      | String                                                                     | `/`                                    | QRM broker virtual host.                                         |
| `QOFFLOAD_REQUEST_QUEUE`         | `queues.request`          | String                                                                     | `qoffload_request_reception_queue`     | Queue for task submission requests.                              |
| `QOFFLOAD_CONTROL_REQUEST_QUEUE` | `queues.control_request`  | String                                                                     | `qoffload_api_request_reception_queue` | Queue for control API requests.                                  |
| `QOFFLOAD_API_REQUEST_QUEUE`     | `queues.control_request`  | String                                                                     | `qoffload_api_request_reception_queue` | Queue for control API requests (_compatibility alias_).          |
| `QOFFLOAD_RESULT_QUEUE`          | `queues.result`           | String                                                                     | `qoffload_result_reception_queue`      | Queue for completed task results.                                |
| `QRM_TASK_QUEUE`                 | `queues.qrm_task`         | String                                                                     | `quantum_task_collection_queue`        | Queue for QRM task submissions.                                  |
| `QOFFLOAD_INSTANCE_UID`          | `identity.instance_uid`   | Unsigned 64-bit integer                                                    | `0`                                    | Used to generate local task identifiers.                         |
| `QOFFLOAD_DAEMON_UID`            | `identity.instance_uid`   | Unsigned 64-bit integer                                                    | `0`                                    | Used to generate local task identifiers (_compatibility alias_). |
| `QOFFLOAD_USER_IDENTITY`         | `identity.user_identity`  | String                                                                     | `qoffload_user`                        | QRM task owner.                                                  |
| `QOFFLOAD_LOG_ENABLED`           | `logging.enabled`         | Boolean                                                                    | `true`                                 | Enables the logging subsystem.                                   |
| `QOFFLOAD_LOG_CONSOLE_ENABLED`   | `logging.console_enabled` | Boolean                                                                    | `true`                                 | Writes log messages to the console.                              |
| `QOFFLOAD_LOG_FILE_ENABLED`      | `logging.file_enabled`    | Boolean                                                                    | `false`                                | Writes log messages to disk files.                               |
| `QOFFLOAD_LOG_LEVEL`             | `logging.level`           | `trace`, `debug`, `info`, `warning`, `warn`, `error`, `critical`, or `off` | `info`                                 | Log verbosity level.                                             |
| `QOFFLOAD_LOG_FILE_MODE`         | `logging.file_mode`       | `append`, `truncate`, or `rotate`                                          | `rotate`                               | File writing strategy.                                           |
| `QOFFLOAD_LOG_ROTATION_SIZE`     | `logging.rotation_size`   | Non-negative size value in bytes                                           | `10,485,760`                           | Maximum log file size before rotation.                           |
| `QOFFLOAD_LOG_ROTATION_FILES`    | `logging.rotation_files`  | Non-negative size value                                                    | `3`                                    | Maximum number of rotated log files.                             |
| `QOFFLOAD_RUNTIME_DIR`           | `paths.runtime_dir`       | Path string                                                                | Build-configured runtime directory     | Directory for runtime-generated files.                           |
| `QOFFLOAD_LOG_DIR`               | `paths.log_dir`           | Path string                                                                | Build-configured log directory         | Directory for stored log files.                                  |
| `QOFFLOAD_STATE_FILE`            | `paths.state_file`        | Path string                                                                | Build-configured state-file path       | Path to persistent application state file.                       |
| `QOFFLOAD_HPC_NODE`              | `policy.hpc_node`         | Boolean                                                                    | `false`                                | Indicates the access path: HPC (`true`) or MQP (`false`).        |
| `QOFFLOAD_RESUME_STATE`          | `policy.resume_state`     | Boolean                                                                    | `false`                                | Restores persisted application state on startup.                 |

If a _compatibility alias_ is defined together with its primary variable, the primary variable takes precedence.
