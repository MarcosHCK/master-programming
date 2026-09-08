# Copyright 2026 MarcosHCK
#
from pathlib import Path

def read_row (line: str, excepts: int = 0):

  pieces = filter (lambda p: len (p) > 0, line.split (' '))
  pieces = tuple ( int (p) for p in pieces )

  if excepts > 0 and len (pieces) != excepts:
    raise Exception ('invalid matrix row length')

  return pieces

with (Path (__file__.removesuffix ('/__main__.py')) / 'example.txt').open ('rt') as stream:

  a, b, c = read_row (stream.readline (), 3)
  d, e, f = read_row (stream.readline (), 3)

print (f'| {a: .2f} {b: .2f} | {c: .2f} |')
print (f'| {d: .2f} {e: .2f} | {f: .2f} |')

if 0 == (di := e * a - d * b):
  raise Exception ('can not solve a singular matrix')

y = (f * a - d * c) / di
x = c / a - (b / a * y)

print (f'x = {x:.2f}, y = {y:.2f}')

print (f'| ax + by = {a * x + b * y: .2f} |')
print (f'| dx + ey = {d * x + e * y: .2f} |')
