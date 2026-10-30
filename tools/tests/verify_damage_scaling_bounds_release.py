"""Verify installed damage normalization cannot narrow2^32 into zero."""
from pathlib import Path
import runpy
native=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
check=runpy.run_path(str(Path(__file__).with_name('damage_scaling_release_check.py')))['check']
check(native,native['instructions'])
