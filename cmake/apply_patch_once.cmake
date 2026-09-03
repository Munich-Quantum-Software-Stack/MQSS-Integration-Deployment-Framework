# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception Copyright (c) MQSS
# Maintainers

# FetchContent PATCH_COMMAND is executed again during re-configuration of an
# existing build tree. Since 'git apply' is not idempotent, re-applying an
# already applied patch would fail.
#
# This helper checks whether the patch is already applied before attempting to
# apply it again.

if(NOT GIT_EXECUTABLE
   OR NOT PATCH_FILE
   OR NOT SOURCE_DIR)
  message(
    FATAL_ERROR
      "Missing required variables: GIT_EXECUTABLE, PATCH_FILE, or SOURCE_DIR")
endif()

execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --check --reverse "${PATCH_FILE}"
  WORKING_DIRECTORY "${SOURCE_DIR}"
  RESULT_VARIABLE reverse_check_result
  OUTPUT_QUIET ERROR_QUIET)

if(reverse_check_result EQUAL 0)
  message(STATUS "Patch already applied: ${PATCH_FILE}")
else()
  message(STATUS "Applying patch: ${PATCH_FILE}")

  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --ignore-whitespace "${PATCH_FILE}"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE apply_result)

  if(NOT apply_result EQUAL 0)
    message(FATAL_ERROR "Failed to apply patch: ${PATCH_FILE}")
  endif()
endif()
