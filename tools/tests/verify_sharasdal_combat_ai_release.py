"""Verify selected combat AI messages and retained current Sharasdal Release."""
from pathlib import Path
import runpy
from sharasdal_combat_ai_pe import verify
retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_lower_ascent_release.py')))
verify(retained['n'])
