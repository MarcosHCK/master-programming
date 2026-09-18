# Copyright 2026 MarcosHCK
#

__matrix_lib = static_library ('matrix',
  dependencies: [ __utils_dep ],
  include_directories: include_directories ('.'),
  sources: [ 'matrix.cc', 'matrix.h', 'matrixbase.h', 'matrixfile.h', 'matrixoperations.h', 'parsen.h' ])

__matrix_dep = declare_dependency (
  dependencies: __utils_dep,
  include_directories: include_directories ('.'),
  link_with: __matrix_lib)

executable ('solution',
  dependencies: [ argparse_dep, __matrix_dep, __utils_dep ],
  include_directories: include_directories ('.'),
  sources: 'main.cc')