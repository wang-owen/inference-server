project(
    'inference_server',
    'cpp',
    version: '0.1.0',
    meson_version: '>=1.1',
    default_options: [
        'cpp_std=c++20',
        'buildtype=debugoptimized',
        'warning_level=1',
    ],
)

threads_dep = dependency('threads')

catch2_dep = dependency(
    'catch2-with-main',
    required: get_option('tests').enabled() or get_option('benchmarks').enabled(),
    default_options: ['tests=false', 'install=false'],
)
build_tests = catch2_dep.found() and get_option('tests').allowed()
build_benchmarks = catch2_dep.found() and get_option('benchmarks').allowed()

subdir('allocator')
subdir('threadpool')
subdir('netserver')
subdir('minicache')
subdir('inference_srv')
