minicache_inc = include_directories('include')
minicache_lib = static_library(
    'minicache',
    files('src/command_dispatcher.cc', 'src/lru_cache.cc'),
    include_directories: minicache_inc,
    dependencies: [netserver_dep],
)
minicache_dep = declare_dependency(
    link_with: minicache_lib,
    include_directories: minicache_inc,
    dependencies: [netserver_dep],
)

executable('minicache_main', 'main.cc', dependencies: [minicache_dep])

if build_tests
    test(
        'minicache',
        executable(
            'minicache_tests',
            files('tests/command_dispatcher_test.cc', 'tests/lru_cache_test.cc'),
            dependencies: [minicache_dep, catch2_dep],
        ),
    )
endif

if build_benchmarks
    benchmark(
        'minicache',
        executable(
            'minicache_bench',
            'benchmarks/minicache_bench.cc',
            dependencies: [minicache_dep, catch2_dep],
        ),
        args: ['[!benchmark]'],
        timeout: 600,
    )
endif
