# Copyright 2026 MarcosHCK
#
from argparse import ArgumentParser
from pathlib import Path
from random import randint, uniform
from typing import Callable, TextIO, TypeVar

T = TypeVar ('T')

def write (stream: TextIO, n: int, m: int, gen: Callable[[], T], separator: str):

  for _ in range (n):

    for j in range (m):

      if 0 == j:
        stream.write (f'{gen ()}')
      else:
        stream.write (f'{separator}{gen ()}')

    stream.write ('\n')

if __name__ == '__main__':

  parser = ArgumentParser ()

  parser.add_argument ('-o', '--output', default = '-', help = 'Output file to write on', metavar = 'FILE', type = str)
  parser.add_argument ('-n', '--rows', default = 2, help = 'Number of rows to generate (default: 2)', metavar = 'M', type = int)
  parser.add_argument ('-m', '--cols', default = None, help = 'Number of columns to generate (default: same as N)', metavar = 'N', type = int)
  parser.add_argument ('-C', '--ceil', default = 1.0, help = 'Maximum value of generated values (default: 1.0)', metavar = 'MAX', type = float)
  parser.add_argument ('-F', '--floor', default = 0.0, help = 'Maximum value of generated values (default: 0.0)', metavar = 'MIN', type = float)
  parser.add_argument ('-b', '--binary', default = False, help = 'Generate binary values (default: False)', action = 'store_true')
  parser.add_argument ('-s', '--separator', default = ' ', help = 'Separator between columns (default:  )', type = str)

  args = parser.parse_args ()

  n = args.rows
  m = args.cols or n

  max_ = args.ceil
  min_ = args.floor

  gen = (lambda: randint (0, 1)) if args.binary else \
        (lambda: uniform (min_, max_))

  if '-' == (output := args.output):
  
    import sys
    write (sys.stdout, n, m, gen, args.separator)
  else:

    with Path (output).open ('wt') as stream:
      write (stream, n, m, gen, args.separator)