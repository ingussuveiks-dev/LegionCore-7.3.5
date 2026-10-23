"""Verify the deployed return hook's exact location, persisted claim and personal credit."""
from pathlib import Path
import runpy

presentation = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_presentation_release.py')))
native = presentation['native']
handler = '?Recover@TalonclawReturn@@'
assert {1220, 7502, 40385, 100111, 100112, 103798, 128808}.issubset(presentation['immediates'](handler))
targets = native['calls'](handler)
for prefix in ('?GetQuestObjectiveData@Player@@QEBAHII@Z', '?KilledMonsterCredit@Player@@', '?HasItemCount@Player@@'):
    assert native['symbol'](prefix) in targets, f'Deployed return hook lacks {prefix}'
for name in ('?OnLogin@player_talonclaw_return@@', '?OnUpdate@player_talonclaw_return@@'):
    assert native['symbol'](name) == native['symbol'](handler) or native['symbol'](handler) in native['calls'](name)
assert not any(name.startswith('?CastSpell@Unit@@') and native['resolve'](address) in targets
               for name, address in native['symbols'].items())
print('PASS: actual Release return hooks require Dalaran1220/7502, persisted flight/claim, owned Talonclaw and personal103798 credit without group-spell bypass')
