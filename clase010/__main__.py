# Copyright 2026 MarcosHCK
#
from argparse import ArgumentParser
from functools import reduce
from itertools import chain
from typing import Callable, Iterable
import math, random

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

def sigmoid (z):
  """Numerically stable sigmoid: 1 / (1 + e^-z)."""
  if z >= 0:
    return 1.0 / (1.0 + math.exp (-z))
  # For very negative z, e^z is tiny and safe (avoids overflow on e^-z)
  ez = math.exp (z)
  return ez / (1.0 + ez)

def sigmoid_derivative_from_output (a):
  """If a = sigmoid(z), then dz/da... rather da/dz = a * (1 - a)."""
  return a * (1.0 - a)

# ----------------------------------------------------------------------
# The perceptron
# ----------------------------------------------------------------------
class Perceptron:

  def __init__ (self, n_inputs: int, learning_rate: float = 0.5,
                activation: Callable[[float], float] = sigmoid,
                derivative: Callable[[float], float] = sigmoid_derivative_from_output):

    self.act = activation
    self.b = random.uniform (-1.0, 1.0)
    self.der = derivative
    self.lr = learning_rate
    self.w = [ random.uniform (-1.0, 1.0) for _ in range (n_inputs) ]

  # ---------------- forward ----------------
  def forward (self, x: list[float]):
    """Return (z, a) for a single input vector x."""

    z = sum (( x * w for x, w in zip (x, self.w) ), self.b)
    return z, self.act (z)

  def predict (self, x: list[float], threshold: float = 0.5):
    """Hard 0/1 decision."""
    return 1 if self.predict_proba (x) >= threshold else 0

  def predict_proba (self, x: list[float]):
    """Probability that the answer is 1."""
    return self.forward (x) [1]

  # ---------------- backward / update ----------------
  def train_one (self, x: list[float], y: float):
    """One SGD step on one example. Returns that example's loss."""
    _, a = self.forward (x)

    delta = (a - y) * self.der (a)

    for i in range (len (self.w)):
        self.w [i] -= self.lr * delta * x [i]

    self.b -= self.lr * delta
    return 0.5 * (a - y) ** 2

  # ---------------- training loop ----------------
  def fit (self, X: list[list[float]], Y: list[float], epochs: int = 10000, sufficient: float = -math.inf, verbose: bool = True):

    last_avg = math.inf

    for epoch in range (epochs):

      total_loss = sum ( self.train_one (x, y) for x, y in zip (X, Y) )
      avg_loss = total_loss / len(X)

      if verbose and (epoch % 2000 == 0 or epoch == epochs - 1):
        print(f'epoch {epoch:6d} avg loss = {avg_loss:.6f}')

      if abs (last_avg - avg_loss) < sufficient:
        print(f'epoch {epoch:6d} avg loss = {avg_loss:.6f}')
        break

      last_avg = avg_loss

    return self

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

  parser.add_argument ('--epochs', default = 3000, type = int)
  parser.add_argument ('--learning-rate', default = 0.3, type = float)
  parser.add_argument ('-l', '--logic', choices = logics.keys (), default = 'and', type = str)
  parser.add_argument ('-n', '--inputs', default = 2, type = int)
  parser.add_argument ('--sufficient', default = -math.inf, type = float)

  args = parser.parse_args ()

  X = list (( list (( float (i) for i in p )) for p in binary_pairs (args.inputs) ))
  Y = logics [args.logic] (X)

  net = Perceptron (n_inputs = len (X [0]), learning_rate = args.learning_rate)
  print ('Training on AND...\n')

  net.fit (X, Y, epochs = args.epochs, sufficient = args.sufficient)

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