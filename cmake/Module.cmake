# Declares a project module: a static library plus its optional main
# executable, test binary, and benchmark binary, following the layout shared
# by every subdirectory of this repo (include/, src/, main.cc, tests/,
# benchmarks/).
function(inference_server_add_module)
  set(one_value_args NAME MAIN)
  set(multi_value_args SOURCES DEPS TEST_SOURCES BENCH_SOURCES)
  cmake_parse_arguments(MOD "" "${one_value_args}" "${multi_value_args}"
                         ${ARGN})

  set(lib_name ${MOD_NAME}_lib)
  add_library(${lib_name} STATIC ${MOD_SOURCES})
  target_include_directories(${lib_name}
                              PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if(MOD_DEPS)
    target_link_libraries(${lib_name} PUBLIC ${MOD_DEPS})
  endif()

  if(MOD_MAIN)
    add_executable(${MOD_NAME}_main ${MOD_MAIN})
    target_link_libraries(${MOD_NAME}_main PRIVATE ${lib_name})
  endif()

  if(INFERENCE_SERVER_BUILD_TESTS AND MOD_TEST_SOURCES)
    add_executable(${MOD_NAME}_tests ${MOD_TEST_SOURCES})
    target_link_libraries(${MOD_NAME}_tests PRIVATE ${lib_name}
                                                     GTest::gtest_main)
    gtest_discover_tests(${MOD_NAME}_tests)
  endif()

  if(INFERENCE_SERVER_BUILD_BENCHMARKS AND MOD_BENCH_SOURCES)
    add_executable(${MOD_NAME}_bench ${MOD_BENCH_SOURCES})
    target_link_libraries(${MOD_NAME}_bench PRIVATE ${lib_name})
  endif()
endfunction()
