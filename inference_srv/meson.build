inference_srv_inc = include_directories('include')
inference_srv_deps = [allocator_dep, threadpool_dep, netserver_dep]
inference_srv_lib = static_library(
    'inference_srv',
    files('src/batching_queue.cc', 'src/inference_engine.cc'),
    include_directories: inference_srv_inc,
    dependencies: inference_srv_deps,
)
inference_srv_dep = declare_dependency(
    link_with: inference_srv_lib,
    include_directories: inference_srv_inc,
    dependencies: inference_srv_deps,
)

executable('inference_srv_main', 'main.cc', dependencies: [inference_srv_dep])

if build_tests
    test(
        'inference_srv',
        executable(
            'inference_srv_tests',
            files(
                'tests/batching_queue_test.cc',
                'tests/inference_engine_test.cc',
            ),
            dependencies: [inference_srv_dep, catch2_dep],
        ),
    )
endif

if build_benchmarks
    benchmark(
        'inference_srv',
        executable(
            'inference_srv_bench',
            'benchmarks/inference_bench.cc',
            dependencies: [inference_srv_dep, catch2_dep],
        ),
        args: ['[!benchmark]', '--benchmark-samples', '5'],
        timeout: 1200,
    )
endif
