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

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_TSAN AND ENABLE_ASAN)
  message(FATAL_ERROR "ENABLE_TSAN and ENABLE_ASAN are mutually exclusive.")
endif()

if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -g -O1)
  add_link_options(-fsanitize=thread)

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  message(STATUS "ThreadSanitizer (TSan) enabled.")
endif()

if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O1)
  add_link_options(-fsanitize=address)

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  message(STATUS "AddressSanitizer (ASan) enabled.")
endif()

find_package(Threads REQUIRED)

include(cmake/Module.cmake)

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
