# Copyright 2026 MarcosHCK
#
from argparse import ArgumentParser
from clase010.Perceptron import PerceptronClassifier
from clase010.sigmoid import *
from functools import reduce
from itertools import chain, batched
from pathlib import Path
from typing import Callable, Iterable
import json, math

def nand (x: int, y: int):
  return not int.__and__ (x, y)

def nor (x: int, y: int):
  return not int.__or__ (x, y)

logics: dict[str, Callable[[list[list[float]]], list[float]]] = \
{
  'and': lambda X: list (( float (reduce (int.__and__, ( int (i) for i in x ))) for x in X )),
  'or': lambda X: list (( float (reduce (int.__or__, ( int (i) for i in x ))) for x in X )),
  'nand': lambda X: list (( float (reduce (nand, ( int (i) for i in x ))) for x in X )),
  'nor': lambda X: list (( float (reduce (nor, ( int (i) for i in x ))) for x in X )),
  'xor': lambda X: list (( float (reduce (int.__xor__, ( int (i) for i in x ))) for x in X )),
}

def binary_pairs (n: int) -> Iterable[tuple[int,...]]:

  assert (n > 0)

  if 1 == n:

    yield from [ (0,), (1,) ]
  else:

    for p in binary_pairs (n - 1):
      yield tuple (chain (p, (0,)))
      yield tuple (chain (p, (1,)))

if __name__ == '__main__':

  parser = ArgumentParser ()

  parser.add_argument ('--epochs', default = 5000, type = int)
  parser.add_argument ('-h', '--header', default = None, type = str)
  parser.add_argument ('--learning-rate', default = 0.3, type = float)
  parser.add_argument ('-l', '--logic', choices = logics.keys (), default = 'and', type = str)
  parser.add_argument ('-n', '--inputs', default = 2, type = int)
  parser.add_argument ('--sufficient', default = -math.inf, type = float)
  parser.add_argument ('--patience', default = 10, type = int,
                       help = 'epochs without enough loss improvement before stopping; 0 disables')
  parser.add_argument ('--min-delta', default = 1e-6, type = float,
                       help = 'minimum loss decrease counted as an improvement')
  parser.add_argument ('--errors-output', default = None, type = Path,
                       help = 'write per-epoch training errors as a JSON array')

  args = parser.parse_args ()

  if args.patience < 0:
    parser.error ('--patience must be zero or greater')

  if not math.isfinite (args.min_delta) or args.min_delta < 0.0:
    parser.error ('--min-delta must be finite and non-negative')

  X = list (( list (( float (i) for i in p )) for p in binary_pairs (args.inputs) ))
  Y = logics [args.logic] (X)

  net = PerceptronClassifier (n_inputs = len (X [0]), learning_rate = args.learning_rate,
    activation = sigmoid, derivative = sigmoid_derivative_from_output)
  print (f'Training on {str (args.logic).upper ()}...\n')

  errors = net.fit (X, Y, epochs = args.epochs, sufficient = args.sufficient,
    patience = None if 0 == args.patience else args.patience,
    min_delta = args.min_delta)

  if None != args.errors_output:

    with args.errors_output.open ('wt', encoding = 'utf-8') as stream:
      json.dump (errors, stream, indent = 2)
      stream.write ('\n')

  line1 = ' '.join ( f'x{i + 1}' for i in range (len (X [0])) )
  line2 = '-' * len (line1)

  print ('\nResults:')
  print (f' {line1} | target |   prob | pred ')
  print (f' {line2}-+--------+--------+------')

  for x, y in zip (X, Y):
    
    p = net.predict_proba (x)
    l = ' '.join ( f' {i:.0f}' for i in x )

    print (f' {l} | {" " * 5}{y:.0f} | {p:.4f} | {" " * 3}{net.predict(x)}')

  print ()
  print (f'  b = {net.b: .4f}')
  print ('  w = [' + ','.join (( f'{w: .4f}' for w in net.w )) + ' ]')

  if None != (header := args.header):

    def first_then_repeat (first: str, repeat: str):

      yield first
      while True: yield repeat

    with Path (header).open ('wt') as stream:

      decl_end = '};\n'
      decl_start = 'static const float weights [] = {\n'

      weights = ( f'{w:.04f},' for w in chain ([net.b], net.w) )
      weights = ( ' '.join (t) + '\n' for t in batched (weights, 4) )

      lines = chain ([decl_start], ( f'  {w}' for w in weights ), [decl_end])

      stream.writelines (lines)