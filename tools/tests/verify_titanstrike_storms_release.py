"""Verify implementation in the canonical linked image, resolving thunks."""
from pathlib import Path
import runpy
native = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_runes_release.py')))
immediates = native['immediates']
image = native['native']
def scoped(prefix):
    if {1495,1068}.issubset(immediates(prefix)):
        return  # Active was inlined
    active = '?Active@at_titanstrike_spirit_winds@@'
    assert image['symbol'](active) in image['calls'](prefix), 'Missing native scenario gate call'
    assert {1495,1068}.issubset(immediates(active)), 'Wrong native scenario gate'
scoped('?Apply@at_titanstrike_spirit_winds@@')
assert 210942 in immediates('?Apply@at_titanstrike_spirit_winds@@')
scoped('?CalculateDuration@at_titanstrike_spirit_winds@@')
assert 365439 in immediates('?CalculateDuration@at_titanstrike_spirit_winds@@'), 'Missing lower-emitter visual guard'
for prefix in ('?BeforeRemove@at_titanstrike_spirit_winds@@', '?OnUpdate@at_titanstrike_spirit_winds@@'):
    native['native']['symbol'](prefix)
assert {365439,365440,210880}.issubset(immediates('?Update@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'))
native['native']['symbol']('?UpdateAI@npc_sr_spectral_windshaperAI@npc_sr_spectral_windshaper@@')
assert 30000 in immediates('?IsSummonedBy@npc_sr_fierce_windsAI@npc_sr_fierce_winds@@')
assert 1495 in immediates('?GetAI@npc_unga_banana_ship_script@@')
print('PASS: canonical PE has scoped lower-emitter visual guard, both force emitters/lifecycle, lane actors and independent tornado timeout')
