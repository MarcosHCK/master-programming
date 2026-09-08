# Copyright 2026 MarcosHCK
#
from inspect import signature
from typing import Callable, Iterable, TypeVar

T = TypeVar ('T')

def n_combinations (iterable1: Iterable[T], n: int) -> Iterable[Iterable[T]]:

  assert (n > 0)

  if 1 == n:

    yield from ( (n, ) for n in iterable1 )
  else:

    sample = list (iterable1)
    yield from ( (*o, n) for o in n_combinations (sample, n - 1) for n in sample )

T = TypeVar ('T')

def print_table (func: Callable[..., T], op_name: str, result_name: str):

  values = [ 0, 1 ]
  results = len (signature (func).parameters)
  results = n_combinations (values, results)

  print (f'{op_name} de (a, b) as {result_name}')
  print (f'--------------------------')

  for (t, result) in ( (r, func (*r)) for r in results ):
    print (f'{op_name} de {t} es {result}')

operations = [
  (lambda a, b: (a and b, a ^ b), 'suma', '(c, r)'),
  (lambda a: int (not a), 'not', 'r'),
  (lambda a, b: a and b, 'and', 'r'),
  (lambda a, b: a or b, 'or', 'r'),
  (lambda a, b: a ^ b, 'xor', 'r'),
  (lambda a, b: int (not (a and b)), 'nand', 'r'),
  (lambda a, b: int (not (a or b)), 'nor', 'r'),
  (lambda a, b: int (not (a ^ b)), 'xnor', 'r'), ]

for i, (f, n, r) in enumerate (operations):

  if i > 0:
    print ('')

  print_table (f, n, r)
