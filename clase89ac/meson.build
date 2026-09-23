# Copyright 2026 MarcosHCK
#

tbb_dep = dependency ('tbb', required: true)

__matrix_lib = static_library ('matrix',
  dependencies: [ tbb_dep, __utils_dep ],
  include_directories: include_directories ('.'),
  sources: [ 'matrix.cc', 'matrix.h', 'matrixbase.h', 'matrixfile.h', 'matrixoperations.h', 'parsen.h' ])

__matrix_dep = declare_dependency (
  dependencies: [ tbb_dep, __utils_dep ],
  include_directories: include_directories ('.'),
  link_with: __matrix_lib)

gperf_exe = find_program ('gperf')

__operations_lookup_tgt = custom_target ('operations-lookup',
  command: [ gperf_exe, '--output-file', '@OUTPUT@', '-PCI', '@INPUT@' ],
  input: 'operations.stringlist',
  output: 'operations.stringlist.h')

indicators_dep = dependency ('indicators')

executable ('solution',
  cpp_pch: 'pch/main.h',
  dependencies: [ argparse_dep, indicators_dep, __matrix_dep, __utils_dep ],
  include_directories: include_directories ('.'),
  sources: [ 'main.cc', __operations_lookup_tgt ])