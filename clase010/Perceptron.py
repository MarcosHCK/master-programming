# Copyright 2026 MarcosHCK
#
from random import Random
from typing import Callable
import math

class PerceptronClassifier:

  def __init__ (self, n_inputs: int,
                seed: int | None = None,
                learning_rate: float = 0.5,
                activation: Callable[[float], float] = lambda x: x,
                derivative: Callable[[float], float] = lambda _: 1):

    rng = Random (seed)

    self.act = activation
    self.b = rng.uniform (-1.0, 1.0)
    self.der = derivative
    self.lr = learning_rate
    self.w = [ rng.uniform (-1.0, 1.0) for _ in range (n_inputs) ]

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