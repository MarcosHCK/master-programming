# Copyright 2026 MarcosHCK
#
from pathlib import Path
from typing import Callable, Iterable, Iterator, Protocol, TypeVar

def load_matrix (lines: Iterable[str]):

  stride = None
  rows = 0

  for line in lines:

    ps = filter (lambda x: '' != x, line.split (' '))
    ps = list ( float (p) for p in ps )

    if None == stride or len (ps) == stride:

      rows = 1 + rows
      stride = len (ps)
    else:
      raise Exception (f'invalid number of columns for row {rows}')

    yield ps

  if 0 == rows:
    raise Exception ('invalid number of rows')

def load_operation (lines: Iterable[str]):

  it = iter (lines)
  op = None

  def op1_gen ():

    nonlocal op

    while True:

      try:

        if ':' == (line := next (it)) [0]:

          op = line [1:].strip ()
          break
        else:
          yield line

      except StopIteration:
        break

  def op2_gen ():

    while True:

      try:
        yield next (it)

      except StopIteration:
        break

  op1 = list (load_matrix (op1_gen ()))
  op2 = list (load_matrix (op2_gen ()))

  return op1, op, op2

T = TypeVar ('T', contravariant = True)

class SupportsWrite(Protocol[T]):
    def write(self, s: T, /) -> object: ...

def matrix_lines (matrix: list[list[float]]):

  def pr (row: list[float]):

    yield '| '

    for n in row:
      yield f'{n: .02f}'

    yield ' |'

  for row in matrix:
    yield ''.join (pr (row))

from sys import stdout

def print_operation (a: list[list[float]], b: list[list[float]], r: list[list[float]], o: str, file: SupportsWrite[str] = stdout):

  ms = [ a, b, r ]
  st: list[tuple[Iterator[str] | None, int | None]] = list (( (matrix_lines (m), None) for m in ms ))

  def next_p (it: Iterator[str] | None, nc: int) -> tuple[str, Iterator[str] | None, int]:

    if not it:
    
      assert (isinstance (nc, int))
      return ' ' * nc, it, nc
    else:

      try:
        nc = len (p := next (it))
        return p, it, nc

      except StopIteration:
        return next_p (None, nc)

  cr_row = 0
  op_row = max (( len (m) for m in ms )) // 2

  while any (( not not it for it, _ in st )):

    lines: list[str] = []

    for i in range (len (st)):

      p, it, nc = next_p (*st [i]) # type: ignore
      st [i] = (it, nc)
      lines.append (p)

    file.write (lines [0])

    if cr_row != op_row:

      file.write (' ' * 3)
    else:
      file.write (f' {o} ')

    file.write (lines [1])

    if cr_row != op_row:
    
      file.write (' ' * 3)
    else:
      file.write (f' {"="} ')

    cr_row = 1 + cr_row

    file.write (lines [2])
    file.write ('\n')

def linear_operator (a: list[list[float]], b: list[list[float]], op: Callable[[float, float], float]):

  if len (a) != len (b):
    raise Exception (f'incompatible matrix sizes ({len (a)} against {len (b)} rows)')

  if len (a [0]) != len (b [0]):
    raise Exception (f'incompatible matrix sizes ({len (a [0])} against {len (b [0])} columns)')

  def do_cols (a: list[float], b: list[float]):

    for e1, e2 in zip (a, b):
      yield op (e1, e2)

  def do_rows (a: list[list[float]], b: list[list[float]]):
  
    for e1, e2 in zip (a, b):
      yield list (do_cols (e1, e2))

  return list (do_rows (a,  b))

with Path ('clase78.txt').open ('rt') as stream:

  g = ( l.split ('#') [0].strip () for l in stream )
  a, o, b = load_operation (g)

  match o:

    case '+':
      print_operation (a, b, linear_operator (a, b, lambda a, b: a + b), o)

    case '-':
      print_operation (a, b, linear_operator (a, b, lambda a, b: a - b), o)
