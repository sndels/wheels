subdir('containers')

natvis_files = containers_natvis_files

natvis_link_args = []
# Embed natvis into PDB when linking with the MSVC linker
if meson.get_compiler('cpp').get_linker_id() == 'link'
  foreach n : natvis_files
    natvis_link_args += '/NATVIS:' + n.full_path()
  endforeach
endif
