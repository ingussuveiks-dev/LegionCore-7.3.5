"""Verify native spellclick visibility, casting and independent SmartAI conditions."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_pickup_admission_release.py')))
native = previous['native']
lookup = '?GetConditionsForSpellClickEvent@ConditionMgr@@'
evaluate = '?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@'
for caller in ['?canSeeSpellClickOn@Player@@', '?HandleSpellClick@Unit@@']:
    assert native['symbol'](lookup) in native['calls'](caller), f'{caller} bypasses source18 lookup'
    assert native['symbol'](evaluate) in native['calls'](caller), f'{caller} bypasses actual condition evaluation'
assert native['symbol']('?canSeeSpellClickOn@Player@@') in native['calls']('?BuildValuesUpdate@Unit@@'), 'Client NPC flag publication bypasses spellclick visibility'
assert native['symbol']('?GetConditionsForSmartEvent@ConditionMgr@@') in native['calls']('?ProcessEvent@SmartScript@@'), 'SmartAI root conditions not consulted'
assert native['symbol'](evaluate) in native['calls']('?ProcessEvent@SmartScript@@'), 'SmartAI root skips native source22 evaluator'
print('PASS: canonical Release uses native source18 lookup/evaluation for client-visible click and cast, independently rechecking source22 before SmartAI progression; SQL admission is verified separately')
