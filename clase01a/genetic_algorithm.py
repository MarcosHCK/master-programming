# Copyright 2026 MarcosHCK
#
"""A small, didactic genetic optimization algorithm implemented in Python."""

from collections.abc import Callable
from dataclasses import dataclass
from itertools import accumulate
from math import fsum, isfinite
from random import Random
from bisect import bisect_right
from typing import Generic, Literal, TypeVar
from tqdm import tqdm

Individual = TypeVar ('Individual')

SelectionType = Literal [
  'elitist', 'random', 'best-with-all', 'best-with-worse', 'tournament', 'roulette',
]
SELECTION_TYPES: tuple[str, ...] = (
  'elitist',
  'random',
  'best-with-all',
  'best-with-worse',
  'tournament',
  'roulette',
)
TOURNAMENT_SIZE = 3

IndividualGenerator = Callable[[Random], Individual]
FitnessFunction = Callable[[Individual], float]
CrossoverFunction = Callable[[Individual, Individual, Random], tuple[Individual, Individual]]
MutationFunction = Callable[[Individual, Random], Individual]
GenerationCallback = Callable[[int, tuple[Individual, ...]], None]
_Candidate = tuple[Individual, float]


@dataclass (frozen = True)
class OptimizationResult (Generic [Individual]):
  """The best individual found over the course of a run."""

  best_individual: Individual
  best_fitness: float
  generations_completed: int


def _fitness_score (fitness_function: FitnessFunction [Individual], individual: Individual) -> float:
  score = fitness_function (individual)

  if not isinstance (score, float):
    raise TypeError ('fitness_function must return a float')

  if not isfinite (score):
    raise ValueError ('fitness_function must return a finite score')

  return score


def _make_pairs (
  population: list[_Candidate [Individual]],
  selection_type: SelectionType,
  rng: Random,
) -> tuple[list[tuple[_Candidate [Individual], _Candidate [Individual]]], list[_Candidate [Individual]]]:
  """Pair candidates according to a strategy and return any unpaired candidates."""

  if 'tournament' == selection_type:
    tournament_size = min (TOURNAMENT_SIZE, len (population))

    def select_tournament_winner () -> _Candidate [Individual]:
      return max (rng.sample (population, tournament_size), key = lambda candidate: candidate [1])

    pairs = [
      (select_tournament_winner (), select_tournament_winner ())
      for _ in range ((len (population) + 1) // 2)
    ]
    unpaired = []

  elif 'roulette' == selection_type:
    scores = [ candidate [1] for candidate in population ]
    score_scale = max (abs (min (scores)), abs (max (scores)))

    if 0.0 == score_scale:
      weights = [1.0] * len (population)
    elif min (scores) < 0.0:
      # Shift negative scores above zero, after scaling to avoid overflow.
      minimum = min (scores) / score_scale
      weights = [ candidate [1] / score_scale - minimum for candidate in population ]
    else:
      # For non-negative fitness use the usual fitness-proportional weights.
      weights = [ candidate [1] / score_scale for candidate in population ]

    total_weight = fsum (weights)

    if 0.0 == total_weight:
      weights = [1.0] * len (population)
      total_weight = float (len (population))

    cumulative_weights = list (accumulate (weights))

    def select_roulette_winner () -> _Candidate [Individual]:
      target = rng.random () * total_weight
      index = min (bisect_right (cumulative_weights, target), len (population) - 1)
      return population [index]

    pairs = [
      (select_roulette_winner (), select_roulette_winner ())
      for _ in range ((len (population) + 1) // 2)
    ]
    unpaired = []

  elif 'random' == selection_type:
    ordered = list (population)
    rng.shuffle (ordered)

    pairs = [
      (ordered [index], ordered [index + 1])
      for index in range (0, len (ordered) - 1, 2)
    ]
    unpaired = ordered [2 * len (pairs):]

  else:
    ranked = sorted (population, key = lambda candidate: candidate [1], reverse = True)

    if 'elitist' == selection_type:
      pairs = [
        (ranked [index], ranked [index + 1])
        for index in range (0, len (ranked) - 1, 2)
      ]
      unpaired = ranked [2 * len (pairs):]

    elif 'best-with-all' == selection_type:
      if len (ranked) < 2:
        pairs = []
        unpaired = ranked
      else:
        pairs = [ (ranked [0], candidate) for candidate in ranked [1:] ]
        unpaired = []

    elif 'best-with-worse' == selection_type:
      pairs = [
        (ranked [index], ranked [len (ranked) - index - 1])
        for index in range (len (ranked) // 2)
      ]
      middle = len (ranked) // 2
      unpaired = ranked [middle:middle + 1] if len (ranked) % 2 else []

    else:
      raise ValueError (f'unknown selection type: {selection_type!r}')

  return pairs, unpaired


def run_genetic_algorithm (
  generate_individual: IndividualGenerator [Individual],
  fitness_function: FitnessFunction [Individual],
  crossover: CrossoverFunction [Individual],
  mutate: MutationFunction [Individual],
  *,
  population_size: int = 200,
  generations: int = 500,
  selection_type: SelectionType = 'elitist',
  elite_size: int = 0,
  seed: int | None = None,
  on_generation: GenerationCallback [Individual] | None = None,
) -> OptimizationResult [Individual]:
  """Run a genetic optimization and return the best individual found.

  ``generate_individual`` creates one individual and receives the run's random
  number generator. ``fitness_function`` must return a finite float; larger
  scores are considered better. To minimize an objective, return its negative.
  ``crossover`` receives two parents and the random generator, and must return
  exactly two offspring in a tuple. ``mutate`` receives one offspring and the
  same random generator and must return the mutated individual.

  Callbacks should treat their input individuals as immutable and return new
  values rather than modifying parents in place. If a pairing strategy leaves an
  individual unpaired (for example, an odd-sized population), it carries forward
  unchanged. After each generation, only the top ``population_size`` offspring
  and carried individuals remain. In ``best-with-all`` mode, the best individual
  crosses with every other member, so the offspring pool can be larger than the
  configured population.

  ``tournament`` chooses each parent as the best of three randomly sampled
  candidates (or all available candidates for smaller populations). ``roulette``
  samples parents with probability proportional to their non-negative fitness;
  when fitness values are negative, scores are shifted so the lowest score gets
  zero weight. Both strategies select with replacement and make enough pairs to
  produce at least ``population_size`` offspring.

  ``elite_size`` is independent of the ``elitist`` pairing strategy. It is the
  number of highest-scoring current individuals copied unchanged into every next
  generation. The remaining population slots are filled from offspring and any
  unpaired candidates. Set it to zero to disable elite preservation.

  If ``on_generation`` is provided, it is called once for generation zero
  (the initial population) and after each completed generation, receiving the
  zero-based generation number and an immutable tuple of individuals.

  The best result is tracked across all generations even if that individual is
  not selected into a later population. All algorithm and callback randomness
  comes from the supplied seeded generator, making runs reproducible when the
  callbacks use that generator too.
  """

  if population_size <= 0:
    raise ValueError ('population_size must be greater than zero')

  if generations < 0:
    raise ValueError ('generations must be zero or greater')

  if isinstance (elite_size, bool) or not isinstance (elite_size, int):
    raise TypeError ('elite_size must be an integer')

  if elite_size < 0 or elite_size > population_size:
    raise ValueError ('elite_size must be between zero and population_size')

  if selection_type not in SELECTION_TYPES:
    raise ValueError (f'unknown selection type: {selection_type!r}')

  rng = Random (seed)
  population: list[_Candidate [Individual]] = []

  for _ in range (population_size):
    individual = generate_individual (rng)
    population.append ((individual, _fitness_score (fitness_function, individual)))

  best_individual, best_fitness = max (population, key = lambda candidate: candidate [1])

  if on_generation is not None:
    on_generation (0, tuple (candidate [0] for candidate in population))

  for generation in tqdm (range (generations), desc = 'Optimizing', unit = 'generation', leave = False):
    elite = sorted (population, key = lambda candidate: candidate [1], reverse = True) [:elite_size]

    if elite_size == population_size:
      if on_generation is not None:
        on_generation (generation + 1, tuple (candidate [0] for candidate in population))

      continue

    pairs, unpaired = _make_pairs (population, selection_type, rng)
    next_population = list (unpaired)

    for first_parent, second_parent in pairs:
      offspring = crossover (first_parent [0], second_parent [0], rng)

      if not isinstance (offspring, tuple) or 2 != len (offspring):
        raise ValueError ('crossover must return a tuple containing exactly two offspring')

      for child in offspring:
        individual = mutate (child, rng)
        fitness = _fitness_score (fitness_function, individual)
        next_population.append ((individual, fitness))

        if fitness > best_fitness:
          best_individual = individual
          best_fitness = fitness

    next_population = [
      candidate for candidate in next_population
      if not any (candidate is elite_candidate for elite_candidate in elite)
    ]
    remaining_slots = population_size - elite_size

    if len (next_population) < remaining_slots:
      raise RuntimeError ('selection produced too few non-elite candidates to fill the next population')

    next_population = sorted (
      next_population,
      key = lambda candidate: candidate [1],
      reverse = True,
    ) [:remaining_slots]
    population = elite + next_population

    if on_generation is not None:
      on_generation (generation + 1, tuple (candidate [0] for candidate in population))

  return OptimizationResult (best_individual, best_fitness, generations)
