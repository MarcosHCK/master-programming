# Copyright 2026 MarcosHCK
#
from clase01a.__main__ import fitness_function, generate_individual, make_generation_capture, mutate, sbx_crossover, selection_types
from clase01a.animation import _validate_limits, sample_fitness_curve, write_generation_animation
from clase01a.genetic_algorithm import _make_pairs, run_genetic_algorithm
from pathlib import Path
from PIL import Image
from random import Random
from tempfile import TemporaryDirectory
from tempfile import TemporaryDirectory as RealTemporaryDirectory
from unittest.mock import patch
import unittest

class GeneticAlgorithmTests (unittest.TestCase):

  def setUp (self):
    self.progress_patch = patch (
      'clase01a.genetic_algorithm.tqdm',
      side_effect = lambda iterable, **_kwargs: iterable,
    )
    self.progress = self.progress_patch.start ()
    self.frame_progress_patch = patch (
      'clase01a.animation.tqdm',
      side_effect = lambda iterable, **_kwargs: iterable,
    )
    self.frame_progress = self.frame_progress_patch.start ()

  def tearDown (self):
    self.progress_patch.stop ()
    self.frame_progress_patch.stop ()

  def run_with_strategy (self, selection_type: str, population_size: int = 5, seed: int | None = 1):
    values = iter (range (population_size))
    observed_pairs = []

    def generate (_rng: Random) -> int:
      return next (values)

    def crossover (first: int, second: int, _rng: Random) -> tuple[int, int]:
      observed_pairs.append ((first, second))
      return first, second

    result = run_genetic_algorithm (
      generate,
      float,
      crossover,
      lambda individual, _rng: individual,
      population_size = population_size,
      generations = 1,
      selection_type = selection_type, # type: ignore
      seed = seed,
    )

    return result, observed_pairs

  def test_elitist_pairs_adjacent_ranks_and_carries_odd_individual (self):
    _, pairs = self.run_with_strategy ('elitist')

    self.assertEqual (pairs, [(4, 3), (2, 1)])

  def test_best_with_worse_pairs_opposite_ranks (self):
    _, pairs = self.run_with_strategy ('best-with-worse')

    self.assertEqual (pairs, [(4, 0), (3, 1)])

  def test_best_with_all_pairs_best_with_every_other_individual (self):
    _, pairs = self.run_with_strategy ('best-with-all')

    self.assertEqual (pairs, [(4, 3), (4, 2), (4, 1), (4, 0)])

  def test_random_pairs_each_individual_once_and_is_seeded (self):
    _, first_run = self.run_with_strategy ('random', seed = 17)
    _, second_run = self.run_with_strategy ('random', seed = 17)

    self.assertEqual (first_run, second_run)
    paired = [individual for pair in first_run for individual in pair]
    self.assertEqual (len (paired), 4)
    self.assertEqual (len (set (paired)), 4)
    self.assertTrue (set (paired).issubset (set (range (5))))

  def test_tournament_selects_best_of_each_sampled_group (self):
    population = [(index, float (index)) for index in range (5)]

    class ScriptedRandom (Random):
      def __init__ (self):
        super ().__init__ (0)
        self.groups = iter ([
          [population [0], population [4], population [2]],
          [population [1], population [3], population [2]],
          [population [0], population [1], population [2]],
          [population [2], population [4], population [3]],
          [population [0], population [3], population [1]],
          [population [2], population [3], population [4]],
        ])

      def sample (self, _population, _k): # type: ignore
        return next (self.groups)

    pairs, unpaired = _make_pairs (population, 'tournament', ScriptedRandom ())

    self.assertEqual (pairs, [
      (population [4], population [3]),
      (population [2], population [4]),
      (population [3], population [4]),
    ])
    self.assertEqual (unpaired, [])

  def test_roulette_uses_nonnegative_fitness_as_relative_weights (self):
    population = [(index, float (score)) for index, score in enumerate ([1, 3, 6, 10])]

    class ScriptedRandom (Random):
      def __init__ (self):
        super ().__init__ (0)
        self.draws = iter ([0.0, 0.1, 0.25, 0.5])

      def random (self):
        return next (self.draws)

    pairs, unpaired = _make_pairs (population, 'roulette', ScriptedRandom ())

    self.assertEqual (pairs, [
      (population [0], population [1]),
      (population [2], population [3]),
    ])
    self.assertEqual (unpaired, [])

  def test_roulette_shifts_negative_scores_and_handles_equal_scores (self):
    population = [(index, float (score)) for index, score in enumerate ([-4, -2, 0, 2])]

    class ScriptedRandom (Random):
      def __init__ (self, draws):
        super ().__init__ (0)
        self.draws = iter (draws)

      def random (self):
        return next (self.draws)

    pairs, _ = _make_pairs (population, 'roulette', ScriptedRandom ([0.0, 0.99, 0.5, 0.75]))

    self.assertEqual (pairs, [
      (population [1], population [3]),
      (population [3], population [3]),
    ])

    tied_population = [(index, -3.0) for index in range (4)]
    tied_pairs, _ = _make_pairs (
      tied_population,
      'roulette',
      ScriptedRandom ([0.0, 0.3, 0.6, 0.99]),
    )
    self.assertEqual (tied_pairs, [
      (tied_population [0], tied_population [1]),
      (tied_population [2], tied_population [3]),
    ])

  def test_tournament_and_roulette_are_exposed_by_cli (self):
    self.assertIn ('tournament', selection_types)
    self.assertIn ('roulette', selection_types)

  def test_generation_callback_receives_initial_and_completed_populations (self):
    snapshots = []

    run_genetic_algorithm (
      lambda rng: rng.random (),
      lambda individual: float (individual),
      lambda first, second, _rng: (first, second),
      lambda individual, _rng: individual,
      population_size = 4,
      generations = 3,
      seed = 2,
      on_generation = lambda generation, population: snapshots.append ((generation, population)),
    )

    self.assertEqual ([generation for generation, _ in snapshots], [0, 1, 2, 3])
    self.assertTrue (all (4 == len (population) for _, population in snapshots))

  def test_configured_elites_are_preserved_verbatim_each_generation (self):
    class Candidate:
      def __init__ (self, score: float):
        self.score = score

    generated = iter ([Candidate (1.0), Candidate (4.0), Candidate (2.0), Candidate (3.0)])
    snapshots = []

    run_genetic_algorithm (
      lambda _rng: next (generated),
      lambda candidate: candidate.score,
      lambda _first, _second, _rng: (Candidate (-1.0), Candidate (-2.0)),
      lambda child, _rng: child,
      population_size = 4,
      generations = 3,
      elite_size = 2,
      on_generation = lambda generation, population: snapshots.append ((generation, population)),
    )

    initial = snapshots [0][1]
    elites = sorted (initial, key = lambda candidate: candidate.score, reverse = True) [:2]

    for _, population in snapshots [1:]:
      self.assertIs (population [0], elites [0])
      self.assertIs (population [1], elites [1])

  def test_elite_size_equal_to_population_preserves_all_without_breeding (self):
    generated = iter ([1, 2, 3])
    snapshots = []
    crossovers = []

    run_genetic_algorithm (
      lambda _rng: next (generated),
      float,
      lambda first, second, _rng: (crossovers.append ((first, second)) or (first, second)),
      lambda individual, _rng: individual,
      population_size = 3,
      generations = 2,
      elite_size = 3,
      on_generation = lambda generation, population: snapshots.append ((generation, population)),
    )

    self.assertEqual (crossovers, [])
    for generation in (1, 2):
      for before, after in zip (snapshots [generation - 1][1], snapshots [generation][1]):
        self.assertIs (before, after)

  def test_invalid_elite_size_is_rejected (self):
    for elite_size in [-1, 4]:
      with self.subTest (elite_size = elite_size):
        with self.assertRaisesRegex (ValueError, 'elite_size'):
          run_genetic_algorithm (
            lambda _rng: 0,
            float,
            lambda first, second, _rng: (first, second),
            lambda individual, _rng: individual,
            population_size = 3,
            elite_size = elite_size,
          )

    with self.assertRaisesRegex (TypeError, 'integer'):
      run_genetic_algorithm (
        lambda _rng: 0,
        float,
        lambda first, second, _rng: (first, second),
        lambda individual, _rng: individual,
        population_size = 3,
        elite_size = 1.5, # type: ignore
      )

  def test_generation_divider_captures_interval_and_final_generation (self):
    snapshots = []
    capture_generation = make_generation_capture (3, 8, snapshots)

    for generation in range (9):
      capture_generation (generation, (float (generation),))

    self.assertEqual ([generation for generation, _ in snapshots], [0, 3, 6, 8])

  def test_generation_divider_one_captures_every_generation (self):
    snapshots = []
    capture_generation = make_generation_capture (1, 3, snapshots)

    for generation in range (4):
      capture_generation (generation, (float (generation),))

    self.assertEqual ([generation for generation, _ in snapshots], [0, 1, 2, 3])

  def test_animation_uses_requested_quadratic_and_writes_all_frames (self):
    self.assertTrue (isinstance (fitness_function (10.0), float))

    with TemporaryDirectory () as directory:
      output = Path (directory) / 'generations.gif'
      frames_directory = Path (directory) / 'plots'
      frame_count = write_generation_animation (
        [(0, (0.0, 25.0, 50.0)), (1, (30.0, 35.0, 37.5))],
        fitness_function,
        output,
        frame_duration = 60,
        frames_directory = frames_directory,
        workers = 2,
      )

      self.assertEqual (frame_count, 2)
      self.assertTrue (output.is_file ())
      self.assertEqual (len (list (frames_directory.glob ('*.png'))), 2)

      with Image.open (output) as gif:
        self.assertEqual (gif.format, 'GIF')
        self.assertEqual (gif.n_frames, 2) # type: ignore

      self.assertEqual (self.frame_progress.call_args.kwargs ['desc'], 'Rendering frames')
      self.assertEqual (self.frame_progress.call_args.kwargs ['unit'], 'frame')
      self.assertEqual (self.frame_progress.call_args.kwargs ['leave'], False)
      self.assertEqual (self.frame_progress.call_args.kwargs ['total'], 2)

  def test_animation_processes_do_not_need_to_pickle_fitness_callback (self):
    with TemporaryDirectory () as directory:
      output = Path (directory) / 'local-fitness.gif'
      local_fitness = lambda t: -float ((t - 20.0) ** 2)

      frame_count = write_generation_animation (
        [(0, (10.0, 20.0, 30.0)), (1, (19.0, 20.0, 21.0))],
        local_fitness,
        output,
        workers = 2,
      )

      self.assertEqual (frame_count, 2)
      self.assertTrue (output.is_file ())

  def test_default_frame_directory_is_temporary_and_cleaned_up (self):
    with TemporaryDirectory () as directory:
      parent = Path (directory)
      output = parent / 'automatic-cleanup.gif'

      with patch (
        'clase01a.animation.TemporaryDirectory',
        side_effect = lambda prefix: RealTemporaryDirectory (prefix = prefix, dir = parent),
      ):
        frame_count = write_generation_animation (
          [(0, (0.0, 25.0, 50.0)), (1, (30.0, 35.0, 37.5))],
          fitness_function,
          output,
        )

      self.assertEqual (frame_count, 2)
      self.assertTrue (output.is_file ())
      self.assertEqual (list (parent.glob ('clase01a_frames_*')), [])

  def test_animation_can_encode_webm (self):
    from shutil import which

    if which ('ffmpeg') is None:
      self.skipTest ('ffmpeg is required to encode WebM')

    with TemporaryDirectory () as directory:
      output = Path (directory) / 'generations.webm'
      frame_count = write_generation_animation (
        [(0, (0.0, 25.0, 50.0)), (1, (30.0, 35.0, 37.5))],
        fitness_function,
        output,
        frame_duration = 100,
        workers = 2,
      )

      self.assertTrue (output.is_file ())
      self.assertGreater (output.stat ().st_size, 0)
      self.assertEqual (frame_count, 2)

  def test_animation_rejects_invalid_gif_arguments (self):
    with TemporaryDirectory () as directory:
      with self.assertRaisesRegex (ValueError, 'frame_duration'):
        write_generation_animation ([(0, (1.0,))], fitness_function, Path (directory) / 'run.gif', frame_duration = 0)

      with self.assertRaisesRegex (ValueError, r'\.gif'):
        write_generation_animation ([(0, (1.0,))], fitness_function, Path (directory) / 'run.png')

      with self.assertRaisesRegex (ValueError, 'workers'):
        write_generation_animation ([(0, (1.0,))], fitness_function, Path (directory) / 'run.gif', workers = 0)

  def test_axis_limits_are_validated (self):
    self.assertEqual (_validate_limits ('x_limits', (-10.0, 50.0)), (-10.0, 50.0))
    self.assertIsNone (_validate_limits ('y_limits', None))

    for limits in [(1.0, 1.0), (2.0, 1.0), (float ('-inf'), 1.0), (0.0,)]:
      with self.subTest (limits = limits):
        with self.assertRaisesRegex (ValueError, 'x_limits'):
          _validate_limits ('x_limits', limits)

  def test_animation_accepts_explicit_axis_limits (self):
    with TemporaryDirectory () as directory:
      output = Path (directory) / 'limited.gif'
      frame_count = write_generation_animation (
        [(0, (10.0, 20.0)), (1, (15.0, 17.0))],
        fitness_function,
        output,
        x_limits = (5.0, 25.0),
        y_limits = (150.0, 180.0),
        workers = 1,
      )

      self.assertEqual (frame_count, 2)
      self.assertTrue (output.is_file ())

  def test_animation_curve_is_sampled_from_the_supplied_fitness_function (self):
    def custom_fitness (t: float) -> float:
      return 2.0 * t + 3.0

    points, values = sample_fitness_curve (custom_fitness, -2.0, 2.0, samples = 5)

    self.assertEqual (points, [-2.0, -1.0, 0.0, 1.0, 2.0])
    self.assertEqual (values, [custom_fitness (point) for point in points])
    self.assertEqual (values, [-1.0, 1.0, 3.0, 5.0, 7.0])

  def test_best_with_all_truncates_extra_offspring_to_population_size (self):
    generated = iter (range (4))
    observed_pairs = []

    def crossover (first: int, second: int, _rng: Random) -> tuple[int, int]:
      observed_pairs.append ((first, second))
      return first, second

    run_genetic_algorithm (
      lambda _rng: next (generated),
      float,
      crossover,
      lambda individual, _rng: individual,
      population_size = 4,
      generations = 2,
      selection_type = 'best-with-all',
    )

    # Three pairs create six offspring; only [3, 3, 3, 2] survive. The next
    # generation therefore has exactly three best-with-all pairings as well.
    self.assertEqual (observed_pairs, [
      (3, 2), (3, 1), (3, 0),
      (3, 3), (3, 3), (3, 2),
    ])

  def test_single_individual_can_run_without_crossover (self):
    result, pairs = self.run_with_strategy ('best-with-all', population_size = 1)

    self.assertEqual (pairs, [])
    self.assertEqual (result.best_individual, 0)
    self.assertEqual (result.best_fitness, 0.0)

  def test_best_found_is_retained_even_if_not_in_later_population (self):
    generated = iter (range (4))

    def crossover (_first: int, second: int, _rng: Random) -> tuple[int, int]:
      return second, second

    result = run_genetic_algorithm (
      lambda _rng: next (generated),
      float,
      crossover,
      lambda individual, _rng: individual,
      population_size = 4,
      generations = 1,
    )

    self.assertEqual (result.best_individual, 3)
    self.assertEqual (result.best_fitness, 3.0)
    self.assertEqual (result.generations_completed, 1)

  def test_generations_zero_returns_best_initial_individual (self):
    generated = iter ([2, 4, 1])

    result = run_genetic_algorithm (
      lambda _rng: next (generated),
      float,
      lambda first, second, _rng: (first, second),
      lambda individual, _rng: individual,
      population_size = 3,
      generations = 0,
    )

    self.assertEqual (result.best_individual, 4)
    self.assertEqual (result.best_fitness, 4.0)
    self.assertEqual (result.generations_completed, 0)

  def test_invalid_population_size_and_generation_count_are_rejected (self):
    with self.assertRaisesRegex (ValueError, 'population_size'):
      run_genetic_algorithm (lambda _rng: 0, float, lambda a, b, _rng: (a, b), lambda x, _rng: x,
                             population_size = 0)

    with self.assertRaisesRegex (ValueError, 'generations'):
      run_genetic_algorithm (lambda _rng: 0, float, lambda a, b, _rng: (a, b), lambda x, _rng: x,
                             generations = -1)

  def test_unknown_selection_type_is_rejected (self):
    with self.assertRaisesRegex (ValueError, 'unknown selection type'):
      run_genetic_algorithm (lambda _rng: 0, float, lambda a, b, _rng: (a, b), lambda x, _rng: x,
                             selection_type = 'unknown') # type: ignore

  def test_fitness_must_be_real_and_finite (self):
    for invalid_score in [True, 1, '1']:
      with self.subTest (score = invalid_score):
        with self.assertRaises (TypeError):
          run_genetic_algorithm (
            lambda _rng: 0,
            lambda _individual: invalid_score,
            lambda a, b, _rng: (a, b),
            lambda individual, _rng: individual,
            population_size = 1,
          )

    for invalid_score in [float ('nan'), float ('inf'), float ('-inf')]:
      with self.subTest (score = invalid_score):
        with self.assertRaisesRegex (ValueError, 'finite'):
          run_genetic_algorithm (
            lambda _rng: 0,
            lambda _individual: invalid_score,
            lambda a, b, _rng: (a, b),
            lambda individual, _rng: individual,
            population_size = 1,
          )

  def test_crossover_must_return_exactly_two_offspring_in_a_tuple (self):
    with self.assertRaisesRegex (ValueError, 'tuple containing exactly two'):
      run_genetic_algorithm (
        lambda _rng: 0,
        float,
        lambda _first, _second, _rng: [0, 0], # type: ignore
        lambda individual, _rng: individual,
        population_size = 2,
        generations = 1,
      )

  def test_progress_bar_is_configured_to_disappear_on_completion (self):
    run_genetic_algorithm (
      lambda _rng: 1.0,
      lambda individual: float (individual),
      lambda first, second, _rng: (first, second),
      lambda individual, _rng: individual,
      population_size = 2,
      generations = 3,
    )

    self.assertEqual (self.progress.call_args.kwargs ['leave'], False)
    self.assertEqual (self.progress.call_args.kwargs ['unit'], 'generation')

  def test_sbx_returns_finite_float_children_and_is_reproducible (self):
    first = sbx_crossover (10.0, 20.0, Random (4))
    second = sbx_crossover (10.0, 20.0, Random (4))

    self.assertEqual (first, second)
    self.assertTrue (all (isinstance (child, float) for child in first))
    self.assertTrue (all (child == child and abs (child) != float ('inf') for child in first))

  def test_sbx_equal_parents_produce_equal_children (self):
    self.assertEqual (sbx_crossover (37.5, 37.5, Random (3)), (37.5, 37.5))

  def test_example_fitness_has_known_maximum (self):
    candidates = [ -100.0 + index * 0.05 for index in range (4001) ]
    optimum = max (candidates, key = fitness_function)
    maximum = fitness_function (optimum)

    self.assertGreater (maximum, fitness_function (optimum - 0.1))
    self.assertGreater (maximum, fitness_function (optimum + 0.1))

  def test_real_valued_example_approaches_known_maximum (self):
    candidates = [ -100.0 + index * 0.05 for index in range (4001) ]
    optimum = max (candidates, key = fitness_function)
    maximum = fitness_function (optimum)

    result = run_genetic_algorithm (
      generate_individual,
      fitness_function,
      sbx_crossover,
      mutate,
      population_size = 120,
      generations = 100,
      seed = 2026,
    )

    self.assertAlmostEqual (result.best_individual, optimum, delta = 0.1)
    self.assertAlmostEqual (result.best_fitness, maximum, delta = 0.01)


if __name__ == '__main__':
  unittest.main ()