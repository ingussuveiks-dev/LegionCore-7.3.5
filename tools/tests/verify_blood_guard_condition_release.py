"""Check the actual native condition dispatch consumed by the one-field SQL fix."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM

p = runpy.run_path(str(Path(__file__).with_name('verify_loading_logout_release.py')))
n, instructions = p['n'], p['instructions']
for caller, target in [
    ('?ProcessEvent@SmartScript@@', '?GetConditionsForSmartEvent@ConditionMgr@@'),
    ('?ProcessEventsFor@SmartScript@@', '?IsObjectMeetingSmartEventConditions@ConditionMgr@@'),
    ('?IsObjectMeetToConditionList@ConditionMgr@@', '?Meets@Condition@@'),
    ('?Meets@Condition@@', '?FindNearestCreature@WorldObject@@'),
    ('?Meets@Condition@@', '?GetScenario@ScenarioMgr@@'),
]:
    assert n['symbol'](target) in n['calls'](caller), f'Missing actual guard condition call {caller} -> {target}'
body = instructions('?Meets@Condition@@')
assert any(op.type == CS_OP_MEM and op.mem.index and op.mem.scale == 8
           for i in body[:65] for op in i.operands), 'Native condition no longer indexes the actual source-target array'
print('PASS actual Release: raw Smart event/root condition dispatch,ElseGroup->Meets,dynamic source-target array,nearest creature and real scenario lookup remain linked for the Blood guard anchor.')
