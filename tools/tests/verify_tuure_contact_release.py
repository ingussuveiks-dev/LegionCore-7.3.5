"""Verify the retained native click, personal-credit and taxi delivery paths."""
from pathlib import Path
import runpy

p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_captain_release.py')))
n,values=p['n'],p['values']
click='?HandleSpellClick@Unit@@'
# Native click casts ordinary (non-triggered) SpellInfo and then independently
# dispatches OnSpellClick. SQL removes that unconditional action, not the cast.
assert any(s.startswith('?CastSpell@Unit@@') and 'SpellInfo' in s and n['resolve'](a) in n['calls'](click)
           for s,a in n['symbols'].items())
assert n['symbol']('?GetConditionsForSpellClickEvent@ConditionMgr@@') in n['calls'](click)
assert 73 in values('?OnSpellClick@SmartAI@@')
assert n['symbol']('?ProcessEventsFor@SmartScript@@') in n['calls']('?OnSpellClick@SmartAI@@')
assert n['symbol']('?KilledMonsterCredit@Player@@') in n['calls']('?EffectKillCreditPersonal@Spell@@')
assert any(s.startswith('?ActivateTaxiPathTo@Player@@') and n['resolve'](a) in n['calls']('?EffectSendTaxi@Spell@@')
           for s,a in n['symbols'].items())
assert not any(s.startswith('?RewardPlayerAndGroupAtEvent@') and n['resolve'](a) in n['calls']('?EffectKillCreditPersonal@Spell@@')
               for s,a in n['symbols'].items())
print('PASS retained actual native ordinary click cast/conditions, independent SmartAI event73, personal effect90 and native taxi123 delivery; no scripted group grant replaces the removed contact event. SQL/native catalog/host own data and failed-cast coverage; actual flight/client pending.')
