<!--
  Derived from: https://github.com/pseudomuto/protoc-gen-doc
  Original file: resources/markdown.tmpl
  License: MIT (see upstream project)
-->

# Protocol Documentation
<a name="top"></a>

## Table of Contents

- [v1/messages.proto](#v1_messages-proto)
    - [CircuitResult](#mqss-protocol-v1-CircuitResult)
    - [CircuitResult.CountsEntry](#mqss-protocol-v1-CircuitResult-CountsEntry)
    - [QHeartBeat](#mqss-protocol-v1-QHeartBeat)
    - [QResourceInfo](#mqss-protocol-v1-QResourceInfo)
    - [QSRegisterEntry](#mqss-protocol-v1-QSRegisterEntry)
    - [QSRegistrationInfo](#mqss-protocol-v1-QSRegistrationInfo)
    - [QuantumResult](#mqss-protocol-v1-QuantumResult)
    - [QuantumTask](#mqss-protocol-v1-QuantumTask)
  
    - [QsStatus](#mqss-protocol-v1-QsStatus)
  



<a name="v1_messages-proto"></a>
<p align="right"><a href="#top">Top</a></p>

## v1/messages.proto



<a name="mqss-protocol-v1-CircuitResult"></a>

### CircuitResult
Measurement counts for one executed circuit.

Each entry maps measured bitstrings to the number of times they were
observed during repeated execution.
Python representation: dict[str, int].


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| counts | CircuitResult.CountsEntry | repeated |  |






<a name="mqss-protocol-v1-CircuitResult-CountsEntry"></a>

### CircuitResult.CountsEntry



| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| key | string |  |  |
| value | int32 |  |  |






<a name="mqss-protocol-v1-QHeartBeat"></a>

### QHeartBeat
Periodic status message sent by a registered backend instance.

Used for asynchronous status reporting from a quantum server to
the resource manager or monitoring components.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| uid | int32 |  | Unique identifier assigned during registration. |
| qpu_name | string |  | Backend name. |
| queue_name | string |  | Endpoint where this backend receives QuantumTask messages (currently a RabbitMQ queue). |
| status | QsStatus |  | Current status of the resource. |
| queue_length | int32 |  | Current queue length. |






<a name="mqss-protocol-v1-QResourceInfo"></a>

### QResourceInfo
Backend capability information message.

Sent asynchronously by a quantum server at startup to report backend
capabilities such as qubit count and connectivity.
It is currently consumed by the MQSS client.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| name | string |  | Name of the backend or QPU. |
| num_qubits | int32 |  | Number of qubits provided by the backend. |
| connectivity | string |  | Backend connectivity information. Python implementation: Optional[str]. |
| instructions | string |  | Backend instruction information. Python implementation: Optional[str]. |






<a name="mqss-protocol-v1-QSRegisterEntry"></a>

### QSRegisterEntry
Backend registration request sent by a quantum server.

Part of a request/response interaction: QSRegisterEntry -> QSRegistrationInfo

The quantum server sends this message to register (or deregister)
a backend instance with the resource manager. It advertises the
backend instance and the endpoint used to receive QuantumTask
messages from the resource manager (currently a RabbitMQ queue).


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| qpu_name | string |  | Name of the backend or quantum server. |
| queue_name | string |  | Endpoint where this backend receives QuantumTask messages (currently a RabbitMQ queue). |
| control_queue_name | string |  | Endpoint where the resource manager sends QSRegistrationInfo responses for this backend instance (currently a RabbitMQ queue). |
| n_qbits | int32 |  | Number of qubits provided by the backend. |
| qpu_type | int32 |  | Backend type code. Represented as int32 because no stable enumeration is defined. |
| deregister | bool |  | If true, requests deregistration instead of registration. |






<a name="mqss-protocol-v1-QSRegistrationInfo"></a>

### QSRegistrationInfo
Response to a backend registration request.

Part of a request/response interaction: QSRegisterEntry -> QSRegistrationInfo

Sent by the resource manager in reply to a QSRegisterEntry request.
It confirms registration or deregistration and provides runtime
parameters such as the backend identifier and heartbeat settings.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| uid | int32 |  | Unique identifier assigned by the resource manager for the backend. |
| heartbeat_queue | string |  | Endpoint where heartbeat messages should be sent (currently a RabbitMQ queue). |
| heartbeat_interval | int32 |  | Heartbeat interval (unit unspecified). |
| deregistered | bool |  | Indicates whether the backend is deregistered after processing the request. |






<a name="mqss-protocol-v1-QuantumResult"></a>

### QuantumResult
Result of a completed quantum task execution.

Part of the asynchronous task execution flow: QuantumTask -> QuantumResult

A QuantumResult is produced after execution on a backend and carries the
execution outcome back through the system to the result destination.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| task_id | int32 |  | Identifier of the task this result belongs to. |
| results | CircuitResult | repeated | Measurement results for executed circuits. Each entry contains counts for measurement bitstrings produced by repeated circuit execution (shots). Python representation: list[dict[str, int]] |
| destination | string |  | Endpoint where the result message is delivered. |
| execution_status | bool |  | Indicates whether execution completed successfully. |
| executed_qpu | string |  | Backend that executed the task. |
| executed_circuits | string | repeated | Circuits executed for this task, serialized as OpenQASM strings. |
| additional_information | string |  | Additional human-readable diagnostic information, such as status or error messages generated during task processing. |
| execution_time | double |  | Wall-clock execution time in seconds measured by the quantum server. |






<a name="mqss-protocol-v1-QuantumTask"></a>

### QuantumTask
Task submitted for quantum circuit execution.

Part of the asynchronous task execution flow: QuantumTask -> QuantumResult

A QuantumTask carries circuit execution input through the scheduling flow:
it is submitted by the client side, forwarded to the resource manager,
scheduled to a backend, and then dispatched to a quantum server for
execution.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| task_id | int32 |  | Unique task identifier. Python implementation: int. |
| n_qbits | int32 |  | Number of qubits. |
| n_shots | int32 |  | Number of shots. |
| circuit_files | string | repeated | Circuit file references. Earlier implementations also accepted a legacy field "circuit_file". |
| circuit_file_type | string |  | Circuit file format/type. |
| result_destination | string |  | Endpoint where the QuantumResult for this task should be delivered. The value is propagated through the scheduling system and used by a quantum server when publishing the final result. |
| preferred_qpu | string |  | Preferred backend requested by the client. |
| scheduled_qpu | string |  | Backend selected by the scheduler for execution. |
| priority | int32 |  | Scheduling priority. |
| optimisation_level | int32 |  | Optimization level (0-3). |
| no_modify | bool |  | If true, modifications to the task/circuit should be avoided. |
| transpiler_flag | bool |  | Enables/disables transpilation. |
| result_type | int32 |  | Result type code. Represented as int32 because no stable enumeration is defined. |
| submit_time | string |  | Submission time. Python representation: str. |
| circuits_qiskit | google.protobuf.Value | repeated | Circuit representation used during task processing. Not used in the initial client-submitted task. Currently opaque; may be defined later. |
| additional_information | string |  | Additional human-readable diagnostic information, such as status or error messages generated during task processing. |
| restricted_resource_names | string | repeated | List of backends on which this task is allowed to run. |
| user_identity | string |  | User identity. |
| token | string |  | Resource access token. |
| via_hpc | bool |  | True if the task was submitted through the HPC integration; otherwise through the portal interface. |





 <!-- end messages -->


<a name="mqss-protocol-v1-QsStatus"></a>

### QsStatus
Status of a quantum server.
UNSPECIFIED (0) was added to preserve the numeric mapping of the
original Python implementation.

| Name | Number | Description |
| ---- | ------ | ----------- |
| QS_STATUS_UNSPECIFIED | 0 |  |
| QS_STATUS_INIT | 1 |  |
| QS_STATUS_RUNNING | 2 |  |
| QS_STATUS_MAINTENANCE | 3 |  |
| QS_STATUS_CALIBRATION | 4 |  |
| QS_STATUS_CONNECTION_TIMEOUT | 5 |  |


 <!-- end enums -->

 <!-- end HasExtensions -->

 <!-- end services -->



