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

__matrix_lib = static_library ('matrix',
  dependencies: [ __utils_dep ],
  include_directories: include_directories ('.'),
  sources: [ 'matrix.cc', 'matrix.h', 'matrixbase.h', 'matrixfile.h', 'parsen.h' ])

__matrix_dep = declare_dependency (
  dependencies: __utils_dep,
  include_directories: include_directories ('.'),
  link_with: __matrix_lib)

executable ('solution',
  dependencies: [ argparse_dep, __utils_dep ],
  include_directories: include_directories ('.'),
  sources: 'main.cc')