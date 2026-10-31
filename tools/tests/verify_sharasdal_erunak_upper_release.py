"""Verify the current upper intro and retained full Sharasdal PE chain."""
from pathlib import Path
import runpy
n=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_erunak_timers_release.py')))
assert n['new_upper'], 'Release lacks the repaired upper Erunak implementation'
print('PASS current upper Erunak implementation plus retained Sharasdal selected Release chain; actual client traversal remains separate.')
