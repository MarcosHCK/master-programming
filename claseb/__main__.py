# Copyright 2026 MarcosHCK
#
from matplotlib.widgets import TextBox
import matplotlib.pyplot as plt
import numpy as np

M = np.array ([[ 1.0, 0.0 ],
               [ 0.0, 1.0 ]])

square = np.array ([[ 0, 0 ], [ 1, 0 ], [ 1, 1 ],
                    [ 0, 1 ], [ 0, 0 ]])

fig, ax = plt.subplots (figsize = (7, 7))

plt.subplots_adjust (bottom = 0.3)

def redraw (_ = None):

  ax.clear ()
  transformed = (M @ square.T).T

  ax.plot (square [:, 0], square [:, 1], color = 'steelblue', lw = 2, label = 'Original')
  ax.fill (square [:, 0], square [:, 1], color = 'steelblue', alpha = 0.15)

  ax.plot (transformed [:, 0], transformed [:, 1], color = 'crimson', lw = 2, label = 'Transformed')
  ax.fill (transformed [:, 0], transformed [:, 1], color = 'crimson', alpha = 0.15)

  a, b = M [ 0, 0 ], M [ 0, 1 ]
  c, d = M [ 1, 0 ], M [ 1, 1 ]

  ax.quiver (0, 0, a, c, angles = 'xy', scale_units = 'xy', scale = 1, color = 'darkgreen', width = 0.012)
  ax.quiver (0, 0, b, d, angles = 'xy', scale_units = 'xy', scale = 1, color = 'darkorange', width = 0.012)
  ax.text (a, c, '  î', color = 'darkgreen', fontsize = 12)
  ax.text (b, d, '  ĵ', color = 'darkorange', fontsize = 12)

  ax.axhline (0, color = 'black', lw = 0.8)
  ax.axvline (0, color = 'black', lw = 0.8)
  ax.set_aspect ('equal')

  ax.grid (True, linestyle = '--', alpha = 0.5)

  pts_x = np.concatenate ([ square [:, 0], transformed [:, 0], [ a, b ] ])
  pts_y = np.concatenate ([ square [:, 1], transformed [:, 1], [ c, d ] ])

  ax.set_xlim (pts_x.min () - 1, pts_x.max () + 1)
  ax.set_ylim (pts_y.min () - 1, pts_y.max () + 1)

  ax.set_title (f'det = {np.linalg.det (M):.2f}')
  ax.legend (loc = 'lower right')

  fig.canvas.draw_idle ()

def make_callback (i, j):

  def update (text):
    try:
      M [i, j] = float (text)
      redraw ()
    except ValueError:
      pass
  return update

# TextBoxes
ax_a = plt.axes (( 0.15, 0.15, 0.15, 0.05 ))
ax_b = plt.axes (( 0.40, 0.15, 0.15, 0.05 ))
ax_c = plt.axes (( 0.15, 0.05, 0.15, 0.05 ))
ax_d = plt.axes (( 0.40, 0.05, 0.15, 0.05 ))

tb_a = TextBox (ax_a, 'a ', initial = f'{M [ 0, 0 ]}')
tb_b = TextBox (ax_b, 'b ', initial = f'{M [ 0, 1 ]}')
tb_c = TextBox (ax_c, 'c ', initial = f'{M [ 1, 0 ]}')
tb_d = TextBox (ax_d, 'd ', initial = f'{M [ 1, 1 ]}')

tb_a.on_submit (make_callback (0, 0))
tb_b.on_submit (make_callback (0, 1))
tb_c.on_submit (make_callback (1, 0))
tb_d.on_submit (make_callback (1, 1))

redraw ()
plt.show ()