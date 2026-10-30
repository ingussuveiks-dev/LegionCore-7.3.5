"""Retained entry point for the extended original-recipient procession verification."""
from pathlib import Path
import runpy
globals().update(runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_procession_recovery_release.py'))))
