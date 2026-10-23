"""Verify deployed native admission, one-shot consumption and NPC dialogue bridges."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_orestes_farewell_release.py')))
native = previous['native']
instructions = previous['instructions']
action = instructions('?ProcessAction@SmartScript@@')
dispatch = next(index for index, entry in enumerate(action) if entry.mnemonic == 'cmp' and any(operand.type == CS_OP_IMM and operand.imm == 223 for operand in entry.operands))
assert any(entry.mnemonic == 'mov' and len(entry.operands) == 2 and entry.operands[0].type == CS_OP_MEM and entry.operands[0].size == 1 and entry.operands[0].mem.disp == 0x99 and entry.operands[1].type == CS_OP_IMM and entry.operands[1].imm == 1 for entry in action[:dispatch]), 'Reinspect runOnce consumption before target dispatch'
assert previous['action_case'](45) != previous['action_case'](1), 'SET_DATA and TALK actions alias'
for caller, callee in [
    ('?SetData@SmartAI@@', '?ProcessEventsFor@SmartScript@@'),
    ('?ProcessEventsFor@SmartScript@@', '?IsObjectMeetingSmartEventConditions@ConditionMgr@@'),
    ('?ProcessEvent@SmartScript@@', '?GetConditionsForSmartEvent@ConditionMgr@@'),
    ('?Meets@Condition@@', '?FindNearestCreature@WorldObject@@'),
    ('?Meets@Condition@@', '?GetScenario@ScenarioMgr@@'),
    ('?GetTargets@SmartScript@@', '?FindNearestCreature@WorldObject@@'),
    ('?ProcessAction@SmartScript@@', '?GetTargets@SmartScript@@'),
    ('?ProcessAction@SmartScript@@', '?SendChat@CreatureTextMgr@@'),
    ('?OnUpdate@SmartScript@@', '?ProcessEventsFor@SmartScript@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing deployed Herald bridge {caller} -> {callee}'
print('PASS: deployed one-shot is consumed before action target selection; native scenario/nearest admission, SmartAI DATA_SET, text send and timed event bridges remain linked. Data-only repair, no client/audio certification')
