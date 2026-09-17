# Copyright 2026 MarcosHCK
#
import sys
from logging import INFO, basicConfig, getLogger
from pathlib import Path
from subprocess import Popen, PIPE

basicConfig (level = INFO, format = '%(levelname)s: %(message)s')

logger = getLogger (__name__)

if not (native := Path ('meson.native')).exists ():
  raise Exception ('invalid base directory')

subdir = Path (__file__).parent.name

if not (example := (base := Path ('.') / subdir) / 'example.txt').exists ():
  raise Exception (f'missing example.txt file (under {base})')

def wait_subprocess (sub: Popen[str]):

  stdout, stderr = sub.communicate ()

  if 0 != sub.returncode:

    if stdout:
      print (stdout, end = '', file = sys.stderr)

    if stderr:
      print (stderr, end = '', file = sys.stderr)

    raise Exception ('subprocess gave a non zero returncode')

(builddir := Path ('builddir/')).mkdir (exist_ok = True)

logger.info ('Configuring meson project')

setup = Popen ([ 'meson', 'setup', '--native-file', str (native.resolve ()), str (Path ('.').resolve ()) ],
               cwd = str (builddir), stdout = PIPE, stderr = PIPE, text = True)

wait_subprocess (setup)

logger.info ('Compiling meson project')

build = Popen ([ 'meson', 'compile' ],
               cwd = str (builddir), stdout = PIPE, stderr = PIPE, text = True)
wait_subprocess (build)

logger.info ('Executing meson project')

instance = Popen ([ f'{subdir}/solution', str (example.resolve ()) ],
                  cwd = str (builddir), text = True)

wait_subprocess (instance)