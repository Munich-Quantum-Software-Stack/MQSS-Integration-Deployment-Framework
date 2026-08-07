# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Copyright (c) MQSS Maintainers

set(CUDAQ_AUTO_FETCH ON CACHE BOOL "" FORCE)
set(CATALYST_AUTO_FETCH ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(mqssci
  GIT_REPOSITORY https://github.com/Munich-Quantum-Software-Stack/MQSS-Quantum-Compilation-Suite
  GIT_TAG        v2.1.0
)

FetchContent_MakeAvailable(mqssci)