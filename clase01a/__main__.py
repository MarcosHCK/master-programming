# Copyright 2026 MarcosHCK
#
from argparse import ArgumentParser
from collections.abc import Callable
from clase01a.genetic_algorithm import run_genetic_algorithm
from math import exp, isfinite, pow
from os import cpu_count
from pathlib import Path
from random import Random
import sys

INITIAL_MIN = -100.0
INITIAL_MAX = 100.0
SBX_DISTRIBUTION_INDEX = 2.0
MUTATION_RATE = 0.15
MUTATION_SIGMA = 2.0

GAUSSIANS_NUMBER = 20

selection_types = {
  'elitist': 'pair adjacent individuals ranked by fitness',
  'random': 'shuffle the population and pair neighbors',
  'best-with-all': 'pair the best individual with every other individual',
  'best-with-worse': 'pair the best with the worst, then work inward',
  'tournament': 'select each parent as the best of three sampled candidates',
  'roulette': 'select parents with probability proportional to fitness',
}

def generate_individual (rng: Random) -> float:
  """Generate a real-valued candidate across the configured search interval."""

  return rng.uniform (INITIAL_MIN, INITIAL_MAX)


def fitness_function (t: float) -> float:
  """Evaluate the example multimodal sum-of-Gaussians fitness function."""

  N = float (n := GAUSSIANS_NUMBER)

  A = lambda k: 1.0 / k
  m = lambda k: -100.0 + 200.0 * k / (N + 1.0)
  s = lambda _: 100.0 / N

  return sum (A (k) * exp (- pow (t - m (k), 2) / (2.0 * pow (s (k), 2))) for k in range (1, n))

def sbx_crossover (first_parent: float, second_parent: float, rng: Random) -> tuple[float, float]:
  """Apply unbounded simulated binary crossover to two scalar parents.

  The distribution index controls how close children tend to be to their
  parents; a larger value produces less spread. This unbounded form is suitable
  for the real-valued t domain, which has no imposed limits.
  """

  if not isfinite (first_parent) or not isfinite (second_parent):
    raise ValueError ('SBX parents must be finite numbers')

  if first_parent == second_parent:
    return first_parent, second_parent

  draw = rng.random ()

  if draw <= 0.5:
    beta = (2.0 * draw) ** (1.0 / (SBX_DISTRIBUTION_INDEX + 1.0))
  else:
    beta = (1.0 / (2.0 * (1.0 - draw))) ** (1.0 / (SBX_DISTRIBUTION_INDEX + 1.0))

  midpoint = 0.5 * (first_parent + second_parent)
  half_difference = 0.5 * beta * (first_parent - second_parent)

  return midpoint + half_difference, midpoint - half_difference


def mutate (individual: float, rng: Random) -> float:
  """Occasionally perturb t with Gaussian noise to maintain population diversity."""

  if rng.random () < MUTATION_RATE:
    return individual + rng.gauss (0.0, MUTATION_SIGMA)

  return individual


def add_optimizer_arguments (parser: ArgumentParser, generations: int) -> None:

  parser.add_argument ('--generations', default = generations,
                       help = 'Number of generations to run (default: %(default)s)', type = int)
  parser.add_argument ('--population-size', default = 200,
                       help = 'Population pool size (default: %(default)s)', type = int)
  parser.add_argument ('--selection-type', choices = selection_types.keys (), default = 'elitist',
                       help = 'Parent-pairing strategy (default: %(default)s)')
  parser.add_argument ('--elite-size', default = 0, type = int,
                       help = 'number of top individuals copied unchanged into each next generation (default: %(default)s)')
  parser.add_argument ('--seed', default = None, help = 'Seed for reproducible random choices', type = int)


def make_generation_capture (
  divider: int,
  final_generation: int,
  snapshots: list,
) -> Callable[[int, tuple[float, ...]], None]:
  """Capture generation zero, each divider-th generation, and the final one."""

  if divider <= 0:
    raise ValueError ('generation divider must be greater than zero')

  def capture_generation (generation: int, population: tuple[float, ...]) -> None:
    if generation % divider == 0 or generation == final_generation:
      snapshots.append ((generation, population))

  return capture_generation


if __name__ == '__main__':

  parser = ArgumentParser (
    description = 'Use a genetic algorithm to maximize the fitness function defined in this module.')

  add_optimizer_arguments (parser, generations = 500)
  subparsers = parser.add_subparsers (dest = 'command')
  animation_parser = subparsers.add_parser (
    'animate',
    help = 'save a graph of every generation and combine them into a GIF or WebM video',
    description = 'Animate genetic optimization using the fitness_function defined in this module.',
  )
  add_optimizer_arguments (animation_parser, generations = 100)
  animation_parser.add_argument ('--output', default = Path ('genetic_optimization.webm'), type = Path,
                                 help = 'GIF or WebM output path (default: %(default)s; choose a .gif extension for GIF)')
  animation_parser.add_argument ('--frames-directory', default = None, type = Path,
                                 help = 'directory for per-generation PNG graphs to preserve (default: temporary directory, automatically removed)')
  animation_parser.add_argument ('--frame-duration', default = 120, type = int,
                                 help = 'duration of each animation frame in milliseconds (default: %(default)s)')
  animation_parser.add_argument ('--workers', default = max (1, (cpu_count () or 1) - 1), type = int,
                                 help = 'parallel frame-rendering worker processes (default: %(default)s; use 1 to disable parallelism)')
  animation_parser.add_argument ('--x-limits', default = None, metavar = ('MIN', 'MAX'), nargs = 2, type = float,
                                 help = 'fixed horizontal axis limits shared by all frames')
  animation_parser.add_argument ('--y-limits', default = None, metavar = ('MIN', 'MAX'), nargs = 2, type = float,
                                 help = 'fixed vertical axis limits shared by all frames')
  animation_parser.add_argument ('--generation-divider', default = 1, type = int,
                                 help = 'render every Nth generation, always including generation zero and the final generation (default: %(default)s)')

  args = parser.parse_args ()

  if args.generations < 0:
    parser.error ('--generations must be zero or greater')

  if args.population_size <= 0:
    parser.error ('--population-size must be greater than zero')

  if args.elite_size < 0 or args.elite_size > args.population_size:
    parser.error ('--elite-size must be between zero and --population-size')

  if args.command == 'animate':
    from clase01a.animation import write_generation_animation

    if args.frame_duration <= 0:
      parser.error ('--frame-duration must be greater than zero')

    if args.workers <= 0:
      parser.error ('--workers must be greater than zero')

    if args.generation_divider <= 0:
      parser.error ('--generation-divider must be greater than zero')

    snapshots = []
    capture_generation = make_generation_capture (args.generation_divider, args.generations, snapshots)

    try:
      result = run_genetic_algorithm (
        generate_individual,
        fitness_function,
        sbx_crossover,
        mutate,
        population_size = args.population_size,
        generations = args.generations,
        selection_type = args.selection_type,
        elite_size = args.elite_size,
        seed = args.seed,
        on_generation = capture_generation,
      )
      frame_count = write_generation_animation (
        snapshots,
        fitness_function,
        args.output,
        frame_duration = args.frame_duration,
        frames_directory = args.frames_directory,
        workers = args.workers,
        x_limits = None if args.x_limits is None else tuple (args.x_limits),
        y_limits = None if args.y_limits is None else tuple (args.y_limits),
      )
    except (OSError, RuntimeError, TypeError, ValueError) as error:
      parser.error (str (error))

    print (f'Animation written to: {args.output}')
    if args.frames_directory is not None:
      print (f'Generation graphs preserved in: {args.frames_directory}')
    else:
      print ('Temporary generation graphs cleaned up.')
    print (f'Best t: {result.best_individual:.10g}')
    print (f'Best y(t): {result.best_fitness:.10g}')
    print (f'Frames: {frame_count}')
    sys.exit (0)

  try:
    result = run_genetic_algorithm (
      generate_individual,
      fitness_function,
      sbx_crossover,
      mutate,
      population_size = args.population_size,
      generations = args.generations,
      selection_type = args.selection_type,
      elite_size = args.elite_size,
      seed = args.seed,
    )
  except (TypeError, ValueError) as error:
    parser.error (str (error))

  print (f'Best t: {result.best_individual:.10g}')
  print (f'Best y(t): {result.best_fitness:.10g}')
  print (f'Generations completed: {result.generations_completed}')