netserver_inc = include_directories('include')
netserver_lib = static_library(
    'netserver',
    files(
        'src/acceptor.cc',
        'src/channel.cc',
        'src/default_poller.cc',
        'src/event_loop.cc',
        'src/poller_poll.cc',
        'src/tcp_listener.cc',
    ),
    include_directories: netserver_inc,
    dependencies: [threads_dep],
)
netserver_dep = declare_dependency(
    link_with: netserver_lib,
    include_directories: netserver_inc,
    dependencies: [threads_dep],
)

executable('netserver_main', 'main.cc', dependencies: [netserver_dep])

if build_tests
    test(
        'netserver',
        executable(
            'netserver_tests',
            files('tests/acceptor_test.cc', 'tests/tcp_listener_test.cc'),
            dependencies: [netserver_dep, catch2_dep],
        ),
    )
endif

if build_benchmarks
    benchmark(
        'netserver',
        executable(
            'netserver_bench',
            'benchmarks/netserver_bench.cc',
            dependencies: [netserver_dep, catch2_dep],
        ),
        args: ['[!benchmark]'],
        timeout: 600,
    )
endif
