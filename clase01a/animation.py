# Copyright 2026 MarcosHCK
#
"""Render genetic-algorithm generations to PNGs and assemble GIF or WebM."""
from clase01a.genetic_algorithm import FitnessFunction
from collections.abc import Sequence
from concurrent.futures import ProcessPoolExecutor, as_completed
from math import isfinite
from pathlib import Path
from tempfile import TemporaryDirectory
from tqdm import tqdm
import shutil
import subprocess

_FRAME_POINTS = []
_FRAME_VALUES = []
_FRAME_X_MIN = 0.0
_FRAME_X_MAX = 1.0
_FRAME_Y_LIMITS = None


def sample_fitness_curve (
  fitness_function: FitnessFunction [float],
  x_min: float,
  x_max: float,
  *,
  samples: int = 500,
) -> tuple[list[float], list[float]]:
  """Sample the supplied fitness callback over the plot's horizontal range."""

  if samples < 2:
    raise ValueError ('samples must be at least two')

  points = [ x_min + (x_max - x_min) * index / (samples - 1) for index in range (samples) ]
  values = [ fitness_function (point) for point in points ]

  return points, values


def _initialize_frame_worker (points, values, x_min: float, x_max: float, y_limits) -> None:
  global _FRAME_POINTS, _FRAME_VALUES, _FRAME_X_MIN, _FRAME_X_MAX, _FRAME_Y_LIMITS
  _FRAME_POINTS = points
  _FRAME_VALUES = values
  _FRAME_X_MIN = x_min
  _FRAME_X_MAX = x_max
  _FRAME_Y_LIMITS = y_limits


def _render_generation_frame (task: tuple) -> str:
  """Render one generation in a worker process and return its PNG path."""

  frame_path, generation, population, population_values = task
  points = _FRAME_POINTS
  values = _FRAME_VALUES
  x_min = _FRAME_X_MIN
  x_max = _FRAME_X_MAX
  y_limits = _FRAME_Y_LIMITS

  import matplotlib
  matplotlib.use ('Agg')
  from matplotlib import pyplot as plt

  figure, axis = plt.subplots (figsize = (9, 5), layout = 'constrained')

  try:
    axis.plot (points, values, color = 'tab:blue', label = 'fitness function')
    axis.scatter (
      population,
      population_values,
      color = 'tab:orange',
      alpha = 0.75,
      label = 'population',
      zorder = 3,
    )

    if population:
      best_index = max (range (len (population)), key = lambda index: population_values [index])
      axis.scatter (
        [population [best_index]],
        [population_values [best_index]],
        color = 'tab:red',
        marker = '*',
        s = 180,
        label = 'generation best',
        zorder = 4,
      )

    axis.set_title (f'Generation {generation}')
    axis.set_xlabel ('t')
    axis.set_ylabel ('fitness(t)')
    axis.set_xlim (x_min, x_max)
    if y_limits is not None:
      axis.set_ylim (*y_limits)
    axis.grid (True, alpha = 0.25)
    axis.legend (loc = 'best')
    figure.savefig (frame_path, format = 'png', dpi = 90)
  finally:
    plt.close (figure)

  return str (frame_path)


def _make_gif (frame_paths: Sequence[Path], output: Path, frame_duration: int) -> None:
  from PIL import Image

  frames = []

  try:
    for frame_path in frame_paths:
      with Image.open (frame_path) as image:
        frames.append (image.convert ('RGB').copy ())

    frames [0].save (
      output,
      format = 'GIF',
      save_all = True,
      append_images = frames [1:],
      duration = frame_duration,
      loop = 0,
      disposal = 2,
    )
  finally:
    for frame in frames:
      frame.close ()


def _make_webm (frame_paths: Sequence[Path], output: Path, frame_duration: int) -> None:
  ffmpeg = shutil.which ('ffmpeg')

  if ffmpeg is None:
    raise RuntimeError ('WebM output requires ffmpeg to be installed and available on PATH')

  frame_rate = 1000.0 / frame_duration
  command = [
    ffmpeg,
    '-hide_banner',
    '-loglevel', 'error',
    '-y',
    '-framerate', f'{frame_rate:.10g}',
    '-start_number', '0',
    '-i', str (frame_paths [0].parent / 'frame-%05d-generation-*.png'),
    '-c:v', 'libvpx-vp9',
    '-crf', '32',
    '-b:v', '0',
    '-pix_fmt', 'yuv420p',
    '-an',
    str (output),
  ]

  # ffmpeg image-sequence input patterns do not support both an index and a
  # wildcard. Use a temporary, stable sequence of numbered symlinks instead.
  sequence_directory = output.parent / f'.{output.stem}_webm_sequence'
  shutil.rmtree (sequence_directory, ignore_errors = True)
  sequence_directory.mkdir (parents = True, exist_ok = True)

  try:
    for index, source in enumerate (frame_paths):
      (sequence_directory / f'frame-{index:05d}.png').symlink_to (source.resolve ())

    command [command.index ('-i') + 1] = str (sequence_directory / 'frame-%05d.png')
    subprocess.run (command, check = True)
  except subprocess.CalledProcessError as error:
    raise RuntimeError (f'ffmpeg failed to encode WebM (exit status {error.returncode})') from error
  finally:
    shutil.rmtree (sequence_directory, ignore_errors = True)


def write_generation_animation (
  generations: Sequence[tuple[int, Sequence[float]]],
  fitness_function: FitnessFunction [float],
  output: Path,
  *,
  frame_duration: int = 120,
  frames_directory: Path | None = None,
  workers: int = 1,
  x_limits: tuple[float, float] | None = None,
  y_limits: tuple[float, float] | None = None,
) -> int:
  """Save a PNG graph per generation and combine them into a GIF or WebM.

  PNG generation can run concurrently in separate worker processes; each
  process owns its own Matplotlib figure. The frames are kept in generation
  order regardless of worker completion order. If no frame directory is
  supplied, the PNGs are created in a temporary directory that is automatically
  removed after encoding. Returns the frame count.
  """

  if not generations:
    raise ValueError ('at least one generation snapshot is required')

  if frame_duration <= 0:
    raise ValueError ('frame_duration must be greater than zero')

  if workers <= 0:
    raise ValueError ('workers must be greater than zero')

  x_limits = _validate_limits ('x_limits', x_limits)
  y_limits = _validate_limits ('y_limits', y_limits)

  output_format = output.suffix.lower ()

  if output_format not in ('.gif', '.webm'):
    raise ValueError ('animation output path must end in .gif or .webm')

  all_individuals = [ individual for _, population in generations for individual in population ]

  if not all_individuals or not all (isfinite (individual) for individual in all_individuals):
    raise ValueError ('generation snapshots must contain finite individuals')

  output.parent.mkdir (parents = True, exist_ok = True)

  if frames_directory is None:
    with TemporaryDirectory (prefix = 'clase01a_frames_') as temporary_directory:
      return _render_and_encode (
        generations,
        fitness_function,
        output,
        frame_duration,
        Path (temporary_directory),
        workers,
        all_individuals,
        x_limits,
        y_limits,
      )

  frames_directory.mkdir (parents = True, exist_ok = True)

  return _render_and_encode (
    generations,
    fitness_function,
    output,
    frame_duration,
    frames_directory,
    workers,
    all_individuals,
    x_limits,
    y_limits,
  )


def _validate_limits (
  name: str,
  limits: tuple[float, float] | None,
) -> tuple[float, float] | None:

  if limits is None:
    return None

  if 2 != len (limits):
    raise ValueError (f'{name} must contain exactly two values: minimum and maximum')

  minimum, maximum = limits

  if not isfinite (minimum) or not isfinite (maximum) or minimum >= maximum:
    raise ValueError (f'{name} must contain finite values with minimum less than maximum')

  return minimum, maximum


def _render_and_encode (
  generations: Sequence[tuple[int, Sequence[float]]],
  fitness_function: FitnessFunction [float],
  output: Path,
  frame_duration: int,
  frames_directory: Path,
  workers: int,
  all_individuals: Sequence[float],
  x_limits: tuple[float, float] | None,
  y_limits: tuple[float, float] | None,
) -> int:

  output_format = output.suffix.lower ()
  if x_limits is None:
    minimum = min (min (all_individuals), 0.0)
    maximum = max (max (all_individuals), 75.0)
    margin = max ((maximum - minimum) * 0.05, 1.0)
    x_min = minimum - margin
    x_max = maximum + margin
  else:
    x_min, x_max = x_limits

  points, values = sample_fitness_curve (fitness_function, x_min, x_max)
  tasks = []

  for frame_index, (generation, population) in enumerate (generations):
    frame_path = frames_directory / f'frame-{frame_index:05d}-generation-{generation:05d}.png'
    population = tuple (population)
    population_values = tuple (fitness_function (individual) for individual in population)
    tasks.append ((str (frame_path), generation, population, population_values))

  if workers == 1:
    _initialize_frame_worker (points, values, x_min, x_max, y_limits)

    frame_paths = [
      Path (_render_generation_frame (task))
      for task in tqdm (tasks, desc = 'Rendering frames', unit = 'frame', leave = False)
    ]
  else:
    completed = []

    with ProcessPoolExecutor (
      max_workers = workers,
      initializer = _initialize_frame_worker,
      initargs = (points, values, x_min, x_max, y_limits),
    ) as executor:
      futures = {
        executor.submit (_render_generation_frame, task): index
        for index, task in enumerate (tasks)
      }

      for future in tqdm (
        as_completed (futures),
        total = len (tasks),
        desc = 'Rendering frames',
        unit = 'frame',
        leave = False,
      ):
        completed.append ((futures [future], future.result ()))

      frame_paths = [ Path (path) for _, path in sorted (completed) ]

  if output_format == '.gif':
    _make_gif (frame_paths, output, frame_duration)
  else:
    _make_webm (frame_paths, output, frame_duration)

  return len (frame_paths)


# Kept as a convenient import alias for earlier examples/tests.
write_generation_gif = write_generation_animation