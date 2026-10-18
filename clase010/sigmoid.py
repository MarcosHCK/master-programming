# Copyright 2026 MarcosHCK
#
import math

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