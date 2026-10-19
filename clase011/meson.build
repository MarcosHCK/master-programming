# Copyright 2026 MarcosHCK
#

avr_core = get_option ('arduino_avr_core')
avr_variant = get_option ('arduino_avr_variant')
fs = import ('fs')

avr_cpp = find_program ('avr-g++', required: false)
avr_cc = find_program ('avr-gcc', required: false)
simavr = find_program ('simavr', required: false)
avr_gdb = find_program ('avr-gdb', required: false)

avr_tools_available = avr_cpp.found () and avr_cc.found () and simavr.found () and avr_gdb.found ()
avr_core_sources = [
  'CDC.cpp', 'HardwareSerial.cpp', 'HardwareSerial0.cpp', 'HardwareSerial1.cpp',
  'HardwareSerial2.cpp', 'HardwareSerial3.cpp', 'IPAddress.cpp', 'PluggableUSB.cpp',
  'Print.cpp', 'Stream.cpp', 'Tone.cpp', 'USBCore.cpp', 'WMath.cpp', 'WString.cpp',
  'abi.cpp', 'main.cpp', 'new.cpp', 'WInterrupts.c', 'hooks.c', 'wiring.c',
  'wiring_analog.c', 'wiring_digital.c', 'wiring_pulse.c', 'wiring_shift.c',
]
avr_core_available = fs.is_file (avr_core / 'Arduino.h') and fs.is_file (avr_variant / 'pins_arduino.h')

foreach source : avr_core_sources
  avr_core_available = avr_core_available and fs.is_file (avr_core / source)
endforeach

if avr_tools_available and avr_core_available

  common_args = [
    '-g', '-Os', '-mmcu=atmega328p', '-DF_CPU=16000000L',
    '-DARDUINO=10819', '-DARDUINO_ARCH_AVR', '-DARDUINO_AVR_UNO',
    '-ffunction-sections', '-fdata-sections', '-fno-exceptions',
  ]
  common_includes = [ '-I' + avr_core, '-I' + avr_variant ]
  core_objects = []

  foreach source : avr_core_sources
    if source.endswith ('.cpp')
    object = custom_target ('arduino-core-' + source.replace ('.cpp', ''),
      command: [ avr_cpp, common_args, '-std=gnu++11', common_includes,
                 '-c', '@INPUT@', '-o', '@OUTPUT@' ],
      input: avr_core / source,
      output: source.replace ('.cpp', '.o'),
      build_by_default: false)
    core_objects += [ object ]
    else
    object = custom_target ('arduino-core-' + source.replace ('.c', ''),
      command: [ avr_cc, common_args, '-std=gnu11', common_includes,
                 '-c', '@INPUT@', '-o', '@OUTPUT@' ],
      input: avr_core / source,
      output: source.replace ('.c', '.o'),
      build_by_default: false)
    core_objects += [ object ]
    endif
  endforeach

  foreach name : [ 'anemia', 'diabetes', 'obesidad' ]
    object = custom_target (name + '-sketch-object',
      command: [ avr_cpp, common_args, '-std=gnu++11', common_includes,
                 '-include', avr_core / 'Arduino.h', '-x', 'c++',
                 '-c', '@INPUT@', '-o', '@OUTPUT@' ],
      input: name + '.ino',
      output: name + '.o',
      build_by_default: false)
    sketch = custom_target (name + '-sketch',
      command: [ avr_cpp, common_args, '-Wl,--gc-sections', '-o', '@OUTPUT@',
                 object, core_objects, '-lm' ],
      input: object,
      output: name + '.elf',
      build_by_default: false)

    test (name + ' sketch', python,
      args: [ files ('sketchrun.py'), name, sketch ],
      depends: [ sketch ],
      suite: 'clase011',
      is_parallel: false,
      timeout: 120)
  endforeach

else

  message ('Arduino sketch tests are disabled: AVR compiler, simavr/GDB, or Arduino core unavailable')

endif