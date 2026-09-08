# Copyright 2026 MarcosHCK
#

project \
  ( 'master', 'c', 'cpp',

    default_options: [ 'c_std=c23', 'cpp_std=c++26', 'warning_level=3' ],
    license: 'GPL-3.0-or-later',
    license_files: [ 'COPYING' ],
    meson_version: '>= 1.3.2',
    version: '0.0.0')

cc = meson.get_compiler ('c')
cpp = meson.get_compiler ('cpp')

# Common warnings

if cc.get_id () == 'gcc' or cc.get_id () == 'clang'

  warning_c_args = [

    '-Waggregate-return',
    '-Warray-bounds',
    '-Wduplicated-branches',
    '-Werror=format=2',
    '-Werror=implicit-function-declaration',
    '-Werror=init-self',
    '-Werror=missing-prototypes',
    '-Werror=pointer-arith',
    '-Werror=pointer-sign',
    '-Werror=unused-result',
    '-Wimplicit-fallthrough',
    '-Wmisleading-indentation',
    '-Wmissing-field-initializers',
    # Due to pervasive use of things like GPOINTER_TO_UINT(), we do not support
    # building with -Wbad-function-cast.
    '-Wno-bad-function-cast',
    # Due to pervasive use of things like GPOINTER_TO_UINT(), we do not support
    # building with -Wbad-function-cast.
    '-Wno-cast-function-type',
    # A zero-length format string shouldn't be considered an issue.
    '-Wno-format-zero-length',
    # Due to function casts through (void*) we cannot support -Wpedantic:
    # ./docs/toolchain-requirements.md#Function_pointer_conversions.
    '-Wno-pedantic',
    '-Wno-string-plus-int',
    # Due to maintained deprecated code, we do not want to see unused parameters
    '-Wno-unused-parameter',
    # We explicitly require variadic macros
    '-Wno-variadic-macros',
    '-Wnonnull',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wunused',
    '-Wunused-function',
    '-Wunused-label',
    '-Wunused-value',
  ]

  warning_c_link_args = [

    '-Wl,-z,nodelete',
  ]
else
  error ('Unsupported compiler')
endif

add_project_arguments (cc.get_supported_arguments (warning_c_args), language: 'c')
add_project_link_arguments (cc.get_supported_link_arguments (warning_c_link_args), language: 'c')

add_project_arguments (cpp.get_supported_arguments (warning_c_args), language: 'cpp')
add_project_link_arguments (cpp.get_supported_link_arguments (warning_c_link_args), language: 'cpp')

argparse_dep = dependency ('argparse', required: true)

__utils_lib = static_library ('utility',
  # TODO: device a portable way to link c++23 standard stacktrace
  # implementation
  dependencies: [ cc.find_library ('libstdc++exp') ],
  include_directories: include_directories ('.'),
  sources: [ 'exception.cc', 'exception.h' ])

__utils_dep = declare_dependency (
  include_directories: include_directories ('.'),
  link_with: __utils_lib)

executable ('clase56',
  dependencies: [ argparse_dep, __utils_dep ],
  include_directories: include_directories ('.'),
  sources: 'clase56.cc')

summary ({
    'C compiler': cc.get_id (),
    'C++ compiler': cpp.get_id (),
    'debug build': get_option ('debug') ? 'yes' : 'no',
    'host cpu': host_machine.cpu_family (),
    'host endian': host_machine.endian (),
    'host system': host_machine.system (),
  }, section: 'Build environment')

if build_machine.system () != host_machine.system ()

  summary ({
      'build cpu' : build_machine.cpu_family (),
      'build endian' : build_machine.endian (),
      'build system' : build_machine.system (),
    }, section: 'Build environment')
endif
