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
  def fit (self, X: list[list[float]], Y: list[float], epochs: int = 10000,
           sufficient: float = -math.inf, verbose: bool = True, *,
           patience: int | None = 10, min_delta: float = 1e-6,
           validation_data: tuple[list[list[float]], list[float]] | None = None) -> list[float]:

    """Train with early stopping and restore the best observed parameters.

    If validation_data is provided, its mean squared loss controls stopping;
    otherwise training loss is monitored. An improvement must exceed
    min_delta to reset patience. The legacy sufficient argument is accepted
    as an alias for min_delta when explicitly set. Returns the mean training
    loss after each completed epoch, including epochs beyond the best one
    before early stopping.
    """

    if not X or len (X) != len (Y):
      raise ValueError ('X and Y must contain the same non-zero number of examples')

    if epochs <= 0:
      raise ValueError ('epochs must be greater than zero')

    if patience is not None and patience <= 0:
      raise ValueError ('patience must be greater than zero or None')

    if sufficient != -math.inf:
      min_delta = sufficient

    if not math.isfinite (min_delta) or min_delta < 0.0:
      raise ValueError ('min_delta must be a finite number greater than or equal to zero')

    if validation_data is None:
      monitor_X, monitor_Y = X, Y
      monitor_name = 'training'
    else:
      monitor_X, monitor_Y = validation_data
      if not monitor_X or len (monitor_X) != len (monitor_Y):
        raise ValueError ('validation inputs and targets must have the same non-zero number of examples')
      monitor_name = 'validation'

    def average_loss (inputs: list[list[float]], targets: list[float]):

      return sum (
        0.5 * (self.predict_proba (x) - y) ** 2
        for x, y in zip (inputs, targets)
      ) / len (inputs)

    best_loss = math.inf
    best_epoch = 0
    best_b = self.b
    best_w = self.w.copy ()
    epochs_without_improvement = 0
    errors: list[float] = []

    self.stopped_early = False
    self.best_epoch = 0
    self.epochs_trained = 0
    self.best_loss = math.inf

    for epoch in range (epochs):

      for x, y in zip (X, Y):
        self.train_one (x, y)

      train_loss = average_loss (X, Y)
      errors.append (train_loss)
      monitored_loss = train_loss if validation_data is None else average_loss (monitor_X, monitor_Y)

      if monitored_loss < best_loss - min_delta:
        best_loss = monitored_loss
        best_epoch = epoch + 1
        best_b = self.b
        best_w = self.w.copy ()
        epochs_without_improvement = 0
      else:
        epochs_without_improvement += 1

      stop = patience is not None and epochs_without_improvement >= patience
      report = verbose and (epoch % 2000 == 0 or epoch == epochs - 1 or stop)

      if report:
        line = f'epoch {epoch + 1:6d} avg loss = {train_loss:.6f}'
        if validation_data is not None:
          line += f' validation loss = {monitored_loss:.6f}'
        print (line)

      self.epochs_trained = epoch + 1

      if stop:
        self.stopped_early = True
        if verbose:
          print (f'early stopping: no {monitor_name} loss improvement for {patience} epochs '
                 f'(best at epoch {best_epoch})')
        break

    self.b = best_b
    self.w = best_w
    self.best_epoch = best_epoch
    self.best_loss = best_loss

    return errors