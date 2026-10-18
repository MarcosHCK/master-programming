# Copyright 2026 MarcosHCK
#
from argparse import ArgumentParser
from clase010.Perceptron import PerceptronClassifier
from clase010.sigmoid import *
from itertools import batched
from pathlib import Path
import csv, json, math

def read_dataset (file: Path, header: str):

  rows: list[tuple[int, list[float], str]] = []
  has_header = 'yes' == header
  expected_columns = None

  with file.open ('rt', encoding = 'utf-8-sig', newline = '') as stream:

    reader = csv.reader (stream)

    for row in reader:

      line = reader.line_num

      if not row or all ('' == field.strip () for field in row):
        continue

      fields = [ field.strip () for field in row ]

      if None == expected_columns:

        if len (fields) < 2:
          raise ValueError (f'line {line}: expected at least 2 columns (one input and one target), got {len (fields)}')

        expected_columns = len (fields)

        if 'auto' == header:

          try:
            [ float (field) for field in fields [:-1] ]
          except ValueError:
            has_header = True

        if has_header:
          has_header = False
          continue

      elif len (fields) != expected_columns:
        raise ValueError (f'line {line}: expected {expected_columns} columns, got {len (fields)}')

      try:
        inputs = [ float (field) for field in fields [:-1] ]
      except ValueError as error:
        raise ValueError (f'line {line}: inputs must be numeric') from error

      if not all (math.isfinite (value) for value in inputs):
        raise ValueError (f'line {line}: inputs must be finite numbers')

      label = fields [-1]

      if '' == label:
        raise ValueError (f'line {line}: class label cannot be empty')

      rows.append ((line, inputs, label))

  if not rows:
    raise ValueError ('CSV contains no data rows')

  labels = sorted ({ row [2] for row in rows })

  if 2 != len (labels):
    raise ValueError (f'expected exactly 2 distinct class labels, found {len (labels)}')

  return rows, labels

def normalize_inputs (inputs: list[list[float]]):

  if not inputs or not inputs [0]:
    raise ValueError ('inputs must contain at least one example with one or more features')

  n_inputs = len (inputs [0])

  if any (len (row) != n_inputs for row in inputs):
    raise ValueError ('all input vectors must have the same number of features')

  means = [ sum (row [column] for row in inputs) / len (inputs) for column in range (n_inputs) ]
  scales = [
    math.sqrt (sum ((row [column] - means [column]) ** 2 for row in inputs) / len (inputs))
    for column in range (n_inputs)
  ]
  scales = [ scale if 0.0 != scale else 1.0 for scale in scales ]

  normalized = [
    [ (value - mean) / scale for value, mean, scale in zip (row, means, scales) ]
    for row in inputs
  ]

  return normalized, means, scales

if __name__ == '__main__':

  parser = ArgumentParser (description = 'Train a binary perceptron from a CSV with numeric inputs and a class column.',
                           add_help = False)
  parser.add_argument ('--help', action = 'help', help = 'show this help message and exit')
  parser.add_argument ('csv_file', type = Path, help = 'CSV with one or more numeric input columns followed by a class column')
  parser.add_argument ('--csv-header', choices = [ 'auto', 'yes', 'no' ], default = 'auto',
                       help = 'whether the CSV has a header row (default: auto-detect)')
  parser.add_argument ('-h', '--header', default = None, type = Path,
                       help = 'write model parameters to a C header file')
  parser.add_argument ('--epochs', default = 5000, type = int)
  parser.add_argument ('--learning-rate', default = 0.3, type = float)
  parser.add_argument ('--patience', default = 10, type = int,
                       help = 'epochs without enough loss improvement before stopping; 0 disables')
  parser.add_argument ('--min-delta', default = 1e-6, type = float,
                       help = 'minimum loss decrease counted as an improvement')
  parser.add_argument ('--errors-output', default = None, type = Path,
                       help = 'write per-epoch training errors as a JSON array')
  parser.add_argument ('--seed', default = 0, type = int, help = 'random seed for reproducible training')

  args = parser.parse_args ()

  if args.epochs <= 0:
    parser.error ('--epochs must be greater than zero')

  if args.patience < 0:
    parser.error ('--patience must be zero or greater')

  if not math.isfinite (args.min_delta) or args.min_delta < 0.0:
    parser.error ('--min-delta must be finite and non-negative')

  if not math.isfinite (args.learning_rate) or args.learning_rate <= 0.0:
    parser.error ('--learning-rate must be a finite number greater than zero')

  try:
    rows, labels = read_dataset (args.csv_file, args.csv_header)
  except (OSError, csv.Error, ValueError) as error:
    parser.error (str (error))

  inputs = [ row [1] for row in rows ]
  targets = [ float (labels.index (row [2])) for row in rows ]
  normalized, means, scales = normalize_inputs (inputs)

  print (f'Class mapping: {labels [0]} = 0, {labels [1]} = 1')
  print (f'Training on {len (rows)} examples...\n')

  net = PerceptronClassifier (n_inputs = len (inputs [0]), learning_rate = args.learning_rate,
    seed = args.seed, activation = sigmoid, derivative = sigmoid_derivative_from_output)

  errors = net.fit (normalized, targets, epochs = args.epochs,
    patience = None if 0 == args.patience else args.patience,
    min_delta = args.min_delta)

  if None != args.errors_output:

    with args.errors_output.open ('wt', encoding = 'utf-8') as stream:
      json.dump (errors, stream, indent = 2)
      stream.write ('\n')

  if None != args.header:

    with args.header.open ('wt', encoding = 'utf-8') as stream:

      stream.write ('#pragma once\n\n')

      for name, values in [
        ('weights', [ net.b, *net.w ]),
        ('means', means),
        ('scales', scales),
      ]:
        stream.write (f'static const float {name} [] = {{\n')

        for group in batched (values, 4):
          literals = []

          for value in group:
            literal = f'{value:.8g}'
            if '.' not in literal and 'e' not in literal.lower ():
              literal += '.0'
            literals.append (literal + 'f,')

          stream.write ('  ' + ' '.join (literals) + '\n')

        stream.write ('};\n')

  print ('\nResults:')
  headers = [ f'input{i + 1}' for i in range (len (inputs [0])) ]
  headers.extend ([ 'class', 'probability', 'predicted', 'correct' ])
  widths = [ max (len (header), max (len (f'{row [1][i]:g}') for row in rows))
             for i, header in enumerate (headers [:-4]) ]
  widths.extend ([
    max (len ('class'), max (len (row [2]) for row in rows)),
    len ('probability'),
    max (len ('predicted'), max (len (label) for label in labels)),
    len ('correct'),
  ])
  print (' | '.join (header.rjust (width) for header, width in zip (headers, widths)))
  print ('-+-'.join ('-' * width for width in widths))

  correct = 0

  for row, x, target in zip (rows, normalized, targets):

    probability = net.predict_proba (x)
    prediction = net.predict (x)
    predicted_label = labels [prediction]
    is_correct = prediction == int (target)
    correct += is_correct

    values = [ f'{value:g}' for value in row [1] ]
    values.extend ([ row [2], f'{probability:.4f}', predicted_label, 'yes' if is_correct else 'no' ])
    print (' | '.join (value.rjust (width) for value, width in zip (values, widths)))

  print (f'\nAccuracy: {correct}/{len (rows)} ({100.0 * correct / len (rows):.1f}%)')
  print (f'  b = {net.b: .4f}')
  print ('  w = [' + ', '.join ((f'{weight: .4f}' for weight in net.w)) + ' ]')
  print ('  input means = [' + ', '.join ((f'{mean: .4g}' for mean in means)) + ' ]')
  print ('  input scales = [' + ', '.join ((f'{scale: .4g}' for scale in scales)) + ' ]')