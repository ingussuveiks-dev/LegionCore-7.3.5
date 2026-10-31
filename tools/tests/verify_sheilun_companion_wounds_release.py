"""Verify shared companion wounds, retained opening and native damage order."""
from pathlib import Path
import runpy
from damage_scaling_release_check import check

opening=runpy.run_path(str(Path(__file__).with_name('verify_sheilun_taran_intro_release.py')))
n=opening['n']
assert opening['wounds'], 'Shared companion wound handlers are absent from this Release'
check(n,n['instructions'])
