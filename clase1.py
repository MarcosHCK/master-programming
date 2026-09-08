# Copyright 2026 MarcosHCK
#
from functools import reduce
from operator import mul

values = ( float (input (f'Lado {n} (m): ')) for n in [ 'A', 'B', 'C' ] )
volume = reduce (mul, values)

print (f'El volumen es {volume} m^3')
