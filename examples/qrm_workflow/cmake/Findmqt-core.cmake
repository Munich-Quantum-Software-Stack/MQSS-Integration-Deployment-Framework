include(FetchContent)
FetchContent_Declare(
  mqt-core
  GIT_REPOSITORY https://github.com/munich-quantum-toolkit/core.git
  GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(mqt-core)   # populates once, here