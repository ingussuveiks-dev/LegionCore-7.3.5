"""Verify implementation in the canonical linked image, resolving thunks."""
from pathlib import Path
import runpy
native = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_runes_release.py')))
immediates = native['immediates']
assert {1495,1068}.issubset(immediates('?Apply@at_titanstrike_spirit_winds@@')) # Active is inlined
assert 210942 in immediates('?Apply@at_titanstrike_spirit_winds@@')
for prefix in ('?BeforeRemove@at_titanstrike_spirit_winds@@', '?OnUpdate@at_titanstrike_spirit_winds@@'):
    native['native']['symbol'](prefix)
assert {365439,365440,210880}.issubset(immediates('?Update@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'))
native['native']['symbol']('?UpdateAI@npc_sr_spectral_windshaperAI@npc_sr_spectral_windshaper@@')
assert 30000 in immediates('?IsSummonedBy@npc_sr_fierce_windsAI@npc_sr_fierce_winds@@')
assert 1495 in immediates('?GetAI@npc_unga_banana_ship_script@@')
print('PASS: canonical PE has Hunter floor emitters, force lifecycle, lane actors and independent tornado timeout')
