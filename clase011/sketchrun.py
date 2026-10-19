#!/usr/bin/env python3
# Copyright 2026 MarcosHCK
#

from pathlib import Path
import csv
import re
import subprocess
import sys

ROOT = Path (__file__).resolve ().parent

def csv_examples (name):

  with (ROOT / f'{name}.csv').open ('rt', encoding = 'utf-8-sig', newline = '') as stream:
    rows = list (csv.reader (stream))

  if len (rows) < 3:
    raise AssertionError (f'{name}.csv must contain examples from both classes')

  examples = {}

  for row in rows [1:]:
    if len (row) < 2:
      continue
    label = row [-1].strip ()
    if label in { '0', '1' }:
      examples.setdefault (int (label), ','.join (field.strip () for field in row [:-1]))
    if len (examples) == 2:
      break

  if set (examples) != { 0, 1 }:
    raise AssertionError (f'{name}.csv must contain at least one row of each class')

  return examples

def output_line (sketch):

  for number, line in enumerate (sketch.read_text (encoding = 'utf-8').splitlines (), start = 1):
    if 'Serial.print (' in line and 'Z = ' in line:
      return number

  raise AssertionError (f'{sketch.name}: cannot find weighted-sum serial output for GDB breakpoint')

def run_example (firmware, name, breakpoint_line, input_line, expected):

  simulator = subprocess.Popen (
    [ 'simavr', '-g', '-m', 'atmega328p', '-f', '16000000', str (firmware) ],
    stdout = subprocess.DEVNULL,
    stderr = subprocess.DEVNULL)

  try:
    # simavr's CLI exposes UART output but no stdin-to-UART bridge. GDB is used
    # to put the received line in HardwareSerial's ring buffer, as if the UART
    # RX interrupt had filled it.
    commands = [
      'target remote 127.0.0.1:1234',
      'break loop',
      'continue',
      'set variable Serial._rx_buffer_tail = 0',
    ]
    received = (input_line + '\n').encode ('ascii')
    commands.extend (
      f'set variable Serial._rx_buffer[{index}] = {byte}'
      for index, byte in enumerate (received)
    )
    commands.extend ([
      f'set variable Serial._rx_buffer_head = {len (received)}',
      f'break {name}.ino:{breakpoint_line}',
      'continue',
      'print Z',
      'print Y',
    ])

    arguments = [ 'avr-gdb', '-q', '-batch', str (firmware) ]
    for instruction in commands:
      arguments.extend ([ '-ex', instruction ])

    result = subprocess.run (
      arguments,
      check = True,
      capture_output = True,
      text = True,
      timeout = 20)
    output = result.stdout + result.stderr

    if not re.search (r'Breakpoint \d+, loop \(\)', output):
      raise AssertionError (f'{name}: GDB did not reach loop():\n{output}')

    values = re.findall (r'\$\d+ = (-?[0-9]+(?:\.[0-9]+)?(?:e[+-]?\d+)?)', output)
    if len (values) < 2:
      raise AssertionError (f'{name}: could not read Z and Y from GDB:\n{output}')

    z = float (values [-2])
    prediction = int (float (values [-1]))

    if prediction != expected:
      raise AssertionError (
        f'{name}: input {input_line!r} predicted {prediction} (Z={z:.6f}), expected {expected}')

    print (f'PASS {name}: {input_line} => Z={z:.6f}, Y={prediction}')

  finally:
    simulator.kill ()
    try:
      simulator.wait (timeout = 2)
    except subprocess.TimeoutExpired:
      simulator.terminate ()
      simulator.wait ()

def main ():

  if len (sys.argv) != 3:
    raise AssertionError ('expected sketch name and firmware path from the Meson test entry')

  name = sys.argv [1]
  firmware = Path (sys.argv [2])
  sketch = ROOT / f'{name}.ino'

  if not sketch.is_file ():
    raise AssertionError (f'{name!r} is not a sketch in {ROOT}')

  if not firmware.is_file ():
    raise AssertionError (f'firmware does not exist: {firmware}')

  breakpoint = output_line (sketch)
  for expected, input_line in sorted (csv_examples (name).items ()):
    run_example (firmware, name, breakpoint, input_line, expected)

  return 0

if __name__ == '__main__':
  raise SystemExit (main ())