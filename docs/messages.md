<!--
  Derived from: https://github.com/pseudomuto/protoc-gen-doc
  Original file: resources/markdown.tmpl
  License: MIT (see upstream project)
-->

# Protocol Documentation
<a name="top"></a>

## Table of Contents

- [v1/messages.proto](#v1_messages-proto)
    - [APIRequest](#mqss-protocol-v1-APIRequest)
    - [APIResponse](#mqss-protocol-v1-APIResponse)
    - [Backend](#mqss-protocol-v1-Backend)
    - [CancelReasonResponse](#mqss-protocol-v1-CancelReasonResponse)
    - [CircuitResult](#mqss-protocol-v1-CircuitResult)
    - [CircuitResult.CountsEntry](#mqss-protocol-v1-CircuitResult-CountsEntry)
    - [CreateTaskRequest](#mqss-protocol-v1-CreateTaskRequest)
    - [CreateTaskResponse](#mqss-protocol-v1-CreateTaskResponse)
    - [ErrorResponse](#mqss-protocol-v1-ErrorResponse)
    - [ListResourcesRequest](#mqss-protocol-v1-ListResourcesRequest)
    - [PendingTasksResponse](#mqss-protocol-v1-PendingTasksResponse)
    - [QHeartBeat](#mqss-protocol-v1-QHeartBeat)
    - [QResourceInfo](#mqss-protocol-v1-QResourceInfo)
    - [QSRegisterEntry](#mqss-protocol-v1-QSRegisterEntry)
    - [QSRegistrationInfo](#mqss-protocol-v1-QSRegistrationInfo)
    - [QuantumResult](#mqss-protocol-v1-QuantumResult)
    - [QuantumTask](#mqss-protocol-v1-QuantumTask)
    - [QubitPair](#mqss-protocol-v1-QubitPair)
    - [ResourceInfoResponse](#mqss-protocol-v1-ResourceInfoResponse)
    - [ResourceRequest](#mqss-protocol-v1-ResourceRequest)
    - [ResourcesResponse](#mqss-protocol-v1-ResourcesResponse)
    - [TaskRequest](#mqss-protocol-v1-TaskRequest)
    - [TaskResultResponse](#mqss-protocol-v1-TaskResultResponse)
    - [TaskStatusResponse](#mqss-protocol-v1-TaskStatusResponse)
  
    - [ApiTaskStatus](#mqss-protocol-v1-ApiTaskStatus)
    - [BackendStatus](#mqss-protocol-v1-BackendStatus)
    - [BackendType](#mqss-protocol-v1-BackendType)
    - [CircuitFormat](#mqss-protocol-v1-CircuitFormat)
    - [QsStatus](#mqss-protocol-v1-QsStatus)
  



<a name="v1_messages-proto"></a>
<p align="right"><a href="#top">Top</a></p>

## v1/messages.proto



<a name="mqss-protocol-v1-APIRequest"></a>

### APIRequest
API request envelope shared by two protocols.

The typed protocol selects an operation through typed_request and ignores
method, request, and data. The REST-like compatibility protocol uses those
fields instead.

Request/response flow: APIRequest -> APIResponse


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| authorization | string |  | Authorization information, such as a token or credential string. |
| method | string |  | Legacy API method name. Ignored by the typed protocol. |
| request | string |  | Legacy requested API operation or endpoint. Ignored by the typed protocol. |
| data | google.protobuf.Struct |  | Legacy request payload. Ignored by the typed protocol. Python representation: dict. |
| response_queue | string |  | Endpoint where the APIResponse should be delivered (currently a RabbitMQ queue). |
| create_task | CreateTaskRequest |  |  |
| task_status | TaskRequest |  |  |
| task_result | TaskRequest |  |  |
| cancel_reason | TaskRequest |  |  |
| list_resources | ListResourcesRequest |  |  |
| resource_info | ResourceRequest |  |  |
| pending_tasks | ResourceRequest |  |  |






<a name="mqss-protocol-v1-APIResponse"></a>

### APIResponse
API response envelope shared by two protocols.

The typed protocol returns exactly one typed_response. The REST-like
compatibility protocol uses response_body instead.

Request/response flow: APIRequest -> APIResponse

Note: Protobuf serializes the complete APIResponse envelope. This differs
from the Python REST-like implementation, which serializes only
response_body as JSON.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| response_body | google.protobuf.Struct |  | REST-like response payload. Ignored by the typed protocol. Python representation: dict. |
| destination_queue | string |  | Endpoint where the response message is delivered (currently a RabbitMQ queue). |
| create_task | CreateTaskResponse |  |  |
| task_status | TaskStatusResponse |  |  |
| task_result | TaskResultResponse |  |  |
| cancel_reason | CancelReasonResponse |  |  |
| resources | ResourcesResponse |  |  |
| resource_info | ResourceInfoResponse |  |  |
| pending_tasks | PendingTasksResponse |  |  |
| error | ErrorResponse |  |  |






<a name="mqss-protocol-v1-Backend"></a>

### Backend



| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| name | string |  | Unique name of the backend or QPU |
| num_qubits | uint32 |  | Number of qubits provided by the backend |
| type | BackendType |  | Type/Technology of the backend |
| status | BackendStatus |  | Status of the backend |
| queue_length | uint32 |  | Current number of QuantumTasks in queue |
| current_load | float |  | Current load, usually between 0.0 - 1.0 A value of 0.0 indicates no load, while 1.0 indicates full load A value greater than 1.0 indicates overload A value less than 0.0 indicates an error or unknown load |
| queue_name | string |  | Endpoint to receives QuantumTask messages (currently a RabbitMQ queue). |
| instructions | string | repeated | List of instructions supported by the backend |
| connectivity | QubitPair | repeated | Connectivity/Coupling map of the backend, represented as pair of qubit ids |
| supported_circuit_formats | CircuitFormat | repeated | Supported circuit file exchange formats/types, e.g., "qasm", "qir" |






<a name="mqss-protocol-v1-CancelReasonResponse"></a>

### CancelReasonResponse
Typed response returned for a cancellation-reason query.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| cancel_reason | string |  | Human-readable cancellation reason. |






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






<a name="mqss-protocol-v1-CreateTaskRequest"></a>

### CreateTaskRequest
Typed payload for creating a quantum task through the API interface.

Mirrors the payload used by the Python "job" POST operation while giving
the typed protocol an explicit operation-specific request message.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| shots | int32 |  | Number of execution shots. |
| circuit | string | repeated | Circuit file references. |
| circuit_format | string |  | Circuit file format/type. |
| resource_name | string |  | Preferred resource/backend name. |
| no_modify | bool |  | If true, modifications to the task/circuit should be avoided. |






<a name="mqss-protocol-v1-CreateTaskResponse"></a>

### CreateTaskResponse
Typed response returned after creating a task.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| uuid | int32 |  | Identifier assigned to the created task. |






<a name="mqss-protocol-v1-ErrorResponse"></a>

### ErrorResponse
Typed error response.

The Python implementation does not define stable error codes. The message
is therefore diagnostic; clients should use the selected oneof case to
identify an error.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| message | string |  |  |






<a name="mqss-protocol-v1-ListResourcesRequest"></a>

### ListResourcesRequest
Typed request for listing available resources.






<a name="mqss-protocol-v1-PendingTasksResponse"></a>

### PendingTasksResponse
Typed response returned for a pending-job query.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| num_pending_jobs | int32 |  | Number of pending jobs, matching the current Python API name. |






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






<a name="mqss-protocol-v1-QubitPair"></a>

### QubitPair



| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| qubit1 | uint32 |  |  |
| qubit2 | uint32 |  |  |






<a name="mqss-protocol-v1-ResourceInfoResponse"></a>

### ResourceInfoResponse
Typed response returned for a resource-information query.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| name | string |  | Resource/backend name. |
| num_qubits | int32 |  | Number of qubits provided by the resource. |
| online | bool |  | Indicates whether the resource is currently available. |






<a name="mqss-protocol-v1-ResourceRequest"></a>

### ResourceRequest
Typed request for an operation addressing a resource.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| resource_name | string |  | Resource/backend name. |






<a name="mqss-protocol-v1-ResourcesResponse"></a>

### ResourcesResponse
Typed response returned for a resource-list query.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| resources | string | repeated | Available resource names. |






<a name="mqss-protocol-v1-TaskRequest"></a>

### TaskRequest
Typed request for an operation addressing an existing task.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| uuid | int32 |  | Task identifier returned by the create-task operation. |






<a name="mqss-protocol-v1-TaskResultResponse"></a>

### TaskResultResponse
Typed response returned for a task-result query.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| result | CircuitResult | repeated | Measurement results. |
| timestamp_submitted | string |  | Submission timestamp. |
| timestamp_scheduled | string |  | Scheduling timestamp. |
| timestamp_completed | string |  | Completion timestamp. |






<a name="mqss-protocol-v1-TaskStatusResponse"></a>

### TaskStatusResponse
Typed response returned for a task-status query.


| Field | Type | Label | Description |
| ----- | ---- | ----- | ----------- |
| status | ApiTaskStatus |  | Current task status. |





 <!-- end messages -->


<a name="mqss-protocol-v1-ApiTaskStatus"></a>

### ApiTaskStatus
Status of a task managed through the API interface.

| Name | Number | Description |
| ---- | ------ | ----------- |
| API_TASK_STATUS_UNSPECIFIED | 0 |  |
| API_TASK_STATUS_WAITING | 1 |  |
| API_TASK_STATUS_COMPLETED | 2 |  |
| API_TASK_STATUS_CANCELLED | 3 |  |



<a name="mqss-protocol-v1-BackendStatus"></a>

### BackendStatus
Status of a quantum backend

| Name | Number | Description |
| ---- | ------ | ----------- |
| BACKEND_STATUS_UNSPECIFIED | 0 |  |
| BACKEND_STATUS_OFFLINE | 1 |  |
| BACKEND_STATUS_IDLE | 2 |  |
| BACKEND_STATUS_BUSY | 3 |  |
| BACKEND_STATUS_ERROR | 4 |  |
| BACKEND_STATUS_MAINTENANCE | 5 |  |
| BACKEND_STATUS_CALIBRATION | 6 |  |



<a name="mqss-protocol-v1-BackendType"></a>

### BackendType
Type of a quantum backend

| Name | Number | Description |
| ---- | ------ | ----------- |
| BACKEND_TYPE_UNSPECIFIED | 0 |  |
| BACKEND_TYPE_SUPERCONDUCTING | 1 |  |
| BACKEND_TYPE_TRAPPED_ION | 2 |  |
| BACKEND_TYPE_NEUTRAL_ATOM | 3 |  |
| BACKEND_TYPE_PHOTONIC | 4 |  |
| BACKEND_TYPE_SIMULATOR | 5 |  |



<a name="mqss-protocol-v1-CircuitFormat"></a>

### CircuitFormat
Circuit exchange formats

| Name | Number | Description |
| ---- | ------ | ----------- |
| CIRCUIT_FORMAT_UNSPECIFIED | 0 |  |
| CIRCUIT_FORMAT_QASM2 | 1 |  |
| CIRCUIT_FORMAT_QASM3 | 2 |  |
| CIRCUIT_FORMAT_QIR | 3 |  |
| CIRCUIT_FORMAT_QIRBASESTRING | 4 |  |
| CIRCUIT_FORMAT_QIRBASEMODULE | 5 |  |
| CIRCUIT_FORMAT_QIRADAPTIVESTRING | 6 |  |
| CIRCUIT_FORMAT_QIRADAPTIVEMODULE | 7 |  |
| CIRCUIT_FORMAT_CALIBRATION | 8 |  |
| CIRCUIT_FORMAT_QPY | 9 |  |
| CIRCUIT_FORMAT_IQMJSON | 10 |  |
| CIRCUIT_FORMAT_BATCHJOB | 11 |  |



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


