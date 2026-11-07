"""Require the fifth acquisition route after the complete retained PE chain."""
import runpy
from pathlib import Path

checked = runpy.run_path(str(Path(__file__).with_name('verify_acquisition_login_recovery_release.py')))
assert checked['marksman'] is not None, 'Release lacks Marksmanship1489/972 empty-checkpoint recovery'
print('PASS required fifth Marksmanship login route;legacy four-route binaries cannot certify this repair')
