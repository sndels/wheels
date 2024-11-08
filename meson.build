project(
  'wheels',
  'cpp',
  default_options: [
    'cpp_std=c++20',
  ],
)

wyhash_proj = subproject('wyhash')
wyhash = wyhash_proj.get_variable('wyhash_dep')

subdir('natvis')

wheels = declare_dependency(
  dependencies: wyhash,
  link_args: natvis_link_args,
  include_directories: include_directories('include'),
)

subdir('bench')
subdir('tests')
