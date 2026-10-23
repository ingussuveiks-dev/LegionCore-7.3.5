"""Verify existing linked native LOS/condition/text paths used by migration24."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_claim_dialogue_release.py')))
native = previous['native']
for caller, callee in [
    ('?OnMoveInLineOfSight@SmartScript@@', '?ProcessEventsFor@SmartScript@@'),
    ('?ProcessEventsFor@SmartScript@@', '?IsObjectMeetingSmartEventConditions@ConditionMgr@@'),
    ('?ProcessEventsFor@SmartScript@@', '?ProcessEvent@SmartScript@@'),
    ('?ProcessEvent@SmartScript@@', '?GetConditionsForSmartEvent@ConditionMgr@@'),
    ('?ProcessEvent@SmartScript@@', '?ProcessAction@SmartScript@@'),
    ('?ProcessAction@SmartScript@@', '?ProcessEvent@SmartScript@@'),
    ('?ProcessAction@SmartScript@@', '?SendChat@CreatureTextMgr@@'),
    ('?OnUpdate@SmartScript@@', '?ProcessEventsFor@SmartScript@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing native bridge {caller} -> {callee}'
print('PASS: canonical Release retains native LOS before condition admission, real event-condition lookup, action/LINK/text send and timer dispatch used by scoped arrival data; successful client movement/voice is not asserted')
