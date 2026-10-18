# Copyright 2026 MarcosHCK
#
from argparse import ArgumentParser
from clase010.Perceptron import PerceptronClassifier
from clase010.sigmoid import sigmoid, sigmoid_derivative_from_output
from functools import reduce
from itertools import batched, chain
from pathlib import Path
from typing import Callable, Iterable
import json, math

def nand (x: int, y: int):
  return not int.__and__ (x, y)

def nor (x: int, y: int):
  return not int.__or__ (x, y)

logics: dict[str, Callable[[list[list[float]]], list[float]]] = \
{
  'and': lambda X: list (
    float (reduce (int.__and__, (int (value) for value in row))) for row in X
  ),
  'or': lambda X: list (
    float (reduce (int.__or__, (int (value) for value in row))) for row in X
  ),
  'nand': lambda X: list (
    float (reduce (nand, (int (value) for value in row))) for row in X
  ),
  'nor': lambda X: list (
    float (reduce (nor, (int (value) for value in row))) for row in X
  ),
  'xor': lambda X: list (
    float (reduce (int.__xor__, (int (value) for value in row))) for row in X
  ),
}

def binary_pairs (n: int) -> Iterable[tuple[int, ...]]:

  assert (n > 0)

  if 1 == n:
    yield from [(0,), (1,)]
  else:
    for pair in binary_pairs (n - 1):
      yield tuple (chain (pair, (0,)))
      yield tuple (chain (pair, (1,)))

def write_weights_header (file: Path, bias: float, weights: list[float]):

  with file.open ('wt', encoding = 'utf-8') as stream:

    stream.write ('#pragma once\n\n')
    stream.write ('static const float weights [] = {\n')

    for group in batched (chain ([bias], weights), 4):

      literals = []

      for value in group:
        literal = f'{value:.8g}'
        if '.' not in literal and 'e' not in literal.lower ():
          literal += '.0'
        literals.append (literal + 'f,')

      stream.write ('  ' + ' '.join (literals) + '\n')

    stream.write ('};\n')

if __name__ == '__main__':

  parser = ArgumentParser (
    description = 'Train a perceptron on a generated Boolean logic truth table.',
    add_help = False)

  parser.add_argument ('--help', action = 'help', help = 'show this help message and exit')
  parser.add_argument ('--epochs', default = 5000, type = int,
                       help = 'maximum number of training epochs (default: %(default)s)')
  parser.add_argument ('-h', '--header', default = None, type = Path,
                       help = 'write the learned bias and weights to a C header file')
  parser.add_argument ('--learning-rate', default = 0.3, type = float,
                       help = 'gradient-descent learning rate (default: %(default)s)')
  parser.add_argument ('-l', '--logic', choices = logics.keys (), default = 'and',
                       help = 'Boolean logic function to train (default: %(default)s)')
  parser.add_argument ('-n', '--inputs', default = 2, type = int,
                       help = 'number of Boolean inputs (default: %(default)s)')
  parser.add_argument ('--sufficient', default = -math.inf, type = float,
                       help = 'legacy alias for the minimum loss improvement threshold')
  parser.add_argument ('--patience', default = 10, type = int,
                       help = 'epochs without enough loss improvement before stopping; 0 disables '
                              '(default: %(default)s)')
  parser.add_argument ('--min-delta', default = 1e-6, type = float,
                       help = 'minimum loss decrease counted as an improvement (default: %(default)s)')
  parser.add_argument ('--errors-output', default = None, type = Path,
                       help = 'write per-epoch training errors as a JSON array')

  args = parser.parse_args ()

  if args.epochs <= 0:
    parser.error ('--epochs must be greater than zero')

  if args.inputs <= 0:
    parser.error ('--inputs must be greater than zero')

  if not math.isfinite (args.learning_rate) or args.learning_rate <= 0.0:
    parser.error ('--learning-rate must be a finite number greater than zero')

  if args.patience < 0:
    parser.error ('--patience must be zero or greater')

  if not math.isfinite (args.min_delta) or args.min_delta < 0.0:
    parser.error ('--min-delta must be finite and non-negative')

  if args.sufficient != -math.inf and (
      not math.isfinite (args.sufficient) or args.sufficient < 0.0):
    parser.error ('--sufficient must be -inf or a finite non-negative number')

  inputs = [[float (value) for value in row] for row in binary_pairs (args.inputs)]
  targets = logics [args.logic] (inputs)

  net = PerceptronClassifier (
    n_inputs = len (inputs [0]),
    learning_rate = args.learning_rate,
    activation = sigmoid,
    derivative = sigmoid_derivative_from_output)

  print (f'Training on {args.logic.upper ()}...\n')

  errors = net.fit (
    inputs,
    targets,
    epochs = args.epochs,
    sufficient = args.sufficient,
    patience = None if 0 == args.patience else args.patience,
    min_delta = args.min_delta)

  if None != args.errors_output:
    with args.errors_output.open ('wt', encoding = 'utf-8') as stream:
      json.dump (errors, stream, indent = 2)
      stream.write ('\n')

  line1 = ' '.join (f'x{index + 1}' for index in range (len (inputs [0])))
  line2 = '-' * len (line1)

  print ('\nResults:')
  print (f' {line1} | target |   prob | pred ')
  print (f' {line2}-+--------+--------+------')

  for row, target in zip (inputs, targets):

    probability = net.predict_proba (row)
    values = ' '.join (f'{value:2.0f}' for value in row)
    print (f' {values} | {target:6.0f} | {probability:.4f} | {net.predict (row):4d}')

  print ()
  print (f'  b = {net.b: .4f}')
  print ('  w = [' + ','.join ((f'{weight: .4f}' for weight in net.w)) + ' ]')

  if None != args.header:
    write_weights_header (args.header, net.b, net.w)