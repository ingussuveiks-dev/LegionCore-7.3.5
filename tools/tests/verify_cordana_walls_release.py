from pathlib import Path
import runpy
from cordana_walls_pe import verify
p=runpy.run_path(str(Path(__file__).with_name('verify_fortysixth_ten_release.py')))
loader=p
while 'callbacks'not in loader:loader=loader['p']
loader=loader|p
verify(loader['n'],loader['ins'])
print('PASS linked Cordana horizontal wall:real endX comparison uses the advancing native X spawn argument,authored minus2.5spacing,100336and18000ms retained;complete prior46groups and lifetime guards retained. Full NPC/client traversal separate.')
