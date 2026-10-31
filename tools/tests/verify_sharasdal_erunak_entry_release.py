"""Verify full selected lower Erunak entry and all retained Sharasdal paths."""
from pathlib import Path
import runpy
proof=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_erunak_timers_release.py')))
assert proof['new_entry']
print('PASS selected canonical lower Erunak entry plus retained upper intro timer/owned destruction/full gauntlet/wall/acquisition/boss/wound PE; actual client/full1066 remains separate.')
