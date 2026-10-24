"""Verify the canonical source13 attachment and native nearby-target consumers."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_zakajz_release.py')))
native = previous['native']
for caller, callee in (
    ('?LoadConditions@ConditionMgr@@', '?addToSpellImplicitTargetConditions@ConditionMgr@@'),
    ('?SelectImplicitNearbyTargets@Spell@@', '?SearchNearbyTarget@Spell@@'),
    ('??RWorldObjectSpellTargetCheck@Trinity@@', '?CanTargetQuestObject@WorldObject@@'),
    ('?Meets@Condition@@', '?HasItemOrGemWithIdEquipped@Player@@'),
    ('?Meets@Condition@@', '?GetScenario@ScenarioMgr@@'),
    ('?Meets@Condition@@', '?GetCurrentStep@Scenario@@'),
):
    assert native['symbol'](callee) in native['calls'](caller), f'Missing deployed consumer {caller} -> {callee}'
checks = {address for name, address in native['symbols'].items() if name.startswith('?IsObjectMeetToConditions@ConditionMgr@@')}
assert checks & native['calls']('??RWorldObjectSpellTargetCheck@Trinity@@'), 'Nearby target checker does not evaluate native condition context'
destinations = {address for name, address in native['symbols'].items() if name.startswith('?SetDst@SpellCastTargets@@') and 'WorldObject' in name}
assert destinations & native['calls']('?SelectImplicitNearbyTargets@Spell@@'), 'Nearby selector does not use an actual world-object destination'
print('PASS: canonical loader attaches source13, nearby selector searches/evaluates native quest-object and candidate/caster conditions, selects world-object destination and retains equipped/scenario consumers; SQL rows and client movement checked separately')
