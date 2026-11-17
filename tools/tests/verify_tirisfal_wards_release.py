"""Verify the linked native cosmetic receiver; no client-fidelity claim."""
from pathlib import Path
import runpy

n = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
for caller, callee in [
    ('?AddLegionScripts@@', '?AddSC_tirisfal_wards@@'),
    ('?SetData@WardAI@TirisfalWards@@', '?RemoveAllAuras@Unit@@'),
    ('?SetData@WardAI@TirisfalWards@@', '?SetData@SmartAI@@'),
    ('?UpdateAI@WardAI@TirisfalWards@@', '?UpdateAI@SmartAI@@'),
    ('?Reset@WardAI@TirisfalWards@@', '?Reset@SmartAI@@'),
]:
    assert n['symbol'](callee) in n['calls'](caller), (caller, callee)
targets = n['calls']('?UpdateAI@WardAI@TirisfalWards@@')
for prefix in ('?CastSpell@Unit@@', '?HasAura@Unit@@'):
    assert any(n['resolve'](address) in targets for name, address in n['symbols'].items() if name.startswith(prefix)), prefix
assert b'npc_tirisfal_holy_ward\0' in n['binary']
assert b'native202014 did not apply after' in n['binary']
print('PASS linked Tirisfal ward registration, real aura receipt/cast/retry, cleanup and SmartAI delegation; no player-credit replacement or client visual claim.')
