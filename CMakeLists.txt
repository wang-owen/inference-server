cmake_minimum_required(VERSION 3.20)
project(
  inference_server
  VERSION 0.1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Build type" FORCE)
endif()

option(INFERENCE_SERVER_BUILD_TESTS "Build GoogleTest unit test suites" ON)
option(INFERENCE_SERVER_BUILD_BENCHMARKS
       "Build std::chrono benchmark executables" ON)

find_package(Threads REQUIRED)

# GoogleTest
if(INFERENCE_SERVER_BUILD_TESTS)
  include(FetchContent)
  set(INSTALL_GTEST
      OFF
      CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(GoogleTest)
endif()

add_subdirectory(allocator)
