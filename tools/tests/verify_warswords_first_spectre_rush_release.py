"""Check first Vigfus Spectre Rush and its distinct retreat cancellation policy."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).with_name('verify_warswords_spectre_rush_release.py')),
              init_globals={'first_spectre_rush': True})
