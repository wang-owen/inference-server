threadpool_inc = include_directories('include')
threadpool_lib = static_library(
    'threadpool',
    files('src/thread_pool.cc'),
    include_directories: threadpool_inc,
    dependencies: [threads_dep],
)
threadpool_dep = declare_dependency(
    link_with: threadpool_lib,
    include_directories: threadpool_inc,
    dependencies: [threads_dep],
)

if build_tests
    test(
        'threadpool',
        executable(
            'threadpool_tests',
            files('tests/thread_pool_test.cc'),
            dependencies: [threadpool_dep, catch2_dep],
        ),
    )
endif

if build_benchmarks
    benchmark(
        'threadpool',
        executable(
            'threadpool_bench',
            'benchmarks/threadpool_bench.cc',
            dependencies: [threadpool_dep, catch2_dep],
        ),
        args: ['[!benchmark]'],
        timeout: 600,
    )
endif
