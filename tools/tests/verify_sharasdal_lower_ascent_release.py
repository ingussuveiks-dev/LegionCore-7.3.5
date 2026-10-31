"""Verify lower ascent and the retained selected Sharasdal Release chain."""
from pathlib import Path
import runpy
from sharasdal_lower_ascent_pe import verify
retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_erunak_reply_release.py')))
verify(retained['retained']['n']['n'])
runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_ascent_admission_release.py')))
