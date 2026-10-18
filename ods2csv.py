# Copyright 2026 MarcosHCK
#
from argparse import ArgumentParser
from pandas import read_excel

if __name__ == '__main__':

  parser = ArgumentParser ()

  parser.add_argument ('input', type = str)
  parser.add_argument ('-o', '--output', default = None, type = str)

  args = parser.parse_args ()
  data = read_excel (filename := str (args.input))

  filename = '.'.join (filename.split ('.') [:-1]) + '.csv'
  filename = str (args.output or filename)

  data.to_csv (filename, index = False, header = True)