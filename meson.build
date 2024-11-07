project(
  'wheels',
  'cpp',
  default_options: [
    'cpp_std=gnu++20',
  ],
)

# TODO
# option(WHEELS_BUILD_TESTS "Include tests" ON)
# option(WHEELS_BUILD_BENCHES "Include benchmarks" ON)

wyhash_proj = subproject('wyhash')
wyhash = wyhash_proj.get_variable('wyhash_dep')

benchmark = dependency('benchmark')

catch2_with_main = dependency('catch2-with-main')

# TODO
# subdir(natvis)

wheels = declare_dependency(
  dependencies: wyhash,
  include_directories: include_directories('include'),
)

# TODO: Can we just add these in the MSBuild target?
# target_sources(wheels
#     INTERFACE
#     ${NATVIS_SOURCES}
# )

subdir('bench')
subdir('tests')
