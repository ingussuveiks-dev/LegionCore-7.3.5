"""Verify the current waypoint reply and retained whole Sharasdal PE chain."""
from pathlib import Path
import runpy
from sharasdal_erunak_reply_pe import verify
retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_erunak_upper_release.py')))
verify(retained['n']['n'])
