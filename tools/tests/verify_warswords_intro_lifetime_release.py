"""Require the installed weak-lifetime intro and retain native bubble cleanup."""
from pathlib import Path
import runpy

bubble = runpy.run_path(str(Path(__file__).with_name('verify_warswords_intro_bubble_release.py')))
channel = bubble['previous']
assert channel.get('lifetime_verified') is True, 'Selected executable lacks the new weak AI lifetime implementation'
print('PASS selected weak lifetime implementation and retained final-intro/native blessing/bubble/death/loot/return consumers. Full944/client traversal remains separate.')
