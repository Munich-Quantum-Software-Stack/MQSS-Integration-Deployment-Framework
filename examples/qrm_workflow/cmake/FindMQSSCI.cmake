set(CUDAQ_AUTO_FETCH ON CACHE BOOL "" FORCE)
set(CATALYST_AUTO_FETCH ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(mqssci
  GIT_REPOSITORY https://github.com/akshay9594/MQSS-Passes-Suite.git
  GIT_TAG        62eb2ebfc47446ad5302b1653b8ac8162ecc32d1 
)

FetchContent_MakeAvailable(mqssci)