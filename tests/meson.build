subdir('allocators')
subdir('containers')

tests_sources = (
  allocator_tests_files
  + container_tests_files
  + files(
    'leak_check.cpp',
    'owning_ptr.cpp',
  )
)

catch2_with_main = dependency('catch2-with-main')

test_exe = executable(
  'wheels_test',
  tests_sources,
  dependencies: [
    wheels,
    catch2_with_main,
  ],
)
test('wheels', test_exe)
