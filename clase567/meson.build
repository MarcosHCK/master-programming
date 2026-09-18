# Copyright 2026 MarcosHCK
#

__utils_lib = static_library ('utility',
  # TODO: device a portable way to link c++23 standard stacktrace
  # implementation
  dependencies: [ cc.find_library ('libstdc++exp') ],
  include_directories: include_directories ('.'),
  sources: [ 'exception.cc', 'exception.h' ])

__utils_dep = declare_dependency (
  include_directories: include_directories ('.'),
  link_with: __utils_lib)

executable ('solution',
  dependencies: [ argparse_dep, __utils_dep ],
  include_directories: include_directories ('.'),
  sources: 'main.cc')