allocator_inc = include_directories('include')
allocator_lib = static_library(
    'allocator',
    files('src/arena_allocator.cc', 'src/slab_allocator.cc'),
    include_directories: allocator_inc,
)
allocator_dep = declare_dependency(
    link_with: allocator_lib,
    include_directories: allocator_inc,
)

if build_tests
    test(
        'allocator',
        executable(
            'allocator_tests',
            files(
                'tests/arena_allocator_test.cc',
                'tests/slab_allocator_test.cc',
            ),
            dependencies: [allocator_dep, catch2_dep],
        ),
    )
endif

if build_benchmarks
    benchmark(
        'allocator',
        executable(
            'allocator_bench',
            'benchmarks/allocator_bench.cc',
            dependencies: [allocator_dep, catch2_dep],
        ),
        args: ['[!benchmark]'],
        timeout: 600,
    )
endif
