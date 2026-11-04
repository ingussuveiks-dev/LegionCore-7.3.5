"""Run actual guard receipt proof together with retained Althea/door dispatch."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('verify_althea_intro_release.py')))
