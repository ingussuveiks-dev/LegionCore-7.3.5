"""Selected Merlis intro plus retained original Althea/guard/door/death proofs."""
from pathlib import Path
import runpy
from merlis_intro_pe import verify

previous = runpy.run_path(str(Path(__file__).with_name('verify_althea_intro_release.py')))
verify(previous['n'])
runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_death_release.py')))
runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_intro_release.py')))
