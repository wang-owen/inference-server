add_library(allocator_lib STATIC src/arena_allocator.cc src/slab_allocator.cc)
target_include_directories(allocator_lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(INFERENCE_SERVER_BUILD_TESTS)
  add_executable(allocator_tests tests/arena_allocator_test.cc
                                 tests/slab_allocator_test.cc)
  target_link_libraries(allocator_tests PRIVATE allocator_lib GTest::gtest_main)
  gtest_discover_tests(allocator_tests)
endif()

if(INFERENCE_SERVER_BUILD_BENCHMARKS)
  add_executable(allocator_bench benchmarks/allocator_bench.cc)
  target_link_libraries(allocator_bench PRIVATE allocator_lib)
endif()
