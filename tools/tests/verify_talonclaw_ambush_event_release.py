"""Verify the deployed opt-in Talonclaw EventObject controller and cleanup."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_ambush_scene_admission_release.py')))
native = previous['native']
instructions = previous['instructions']
for caller, callee in [
    ('?IsTriggerMeets@eventobject_talonclaw_ambush@@', '?Eligible@TalonclawAmbushEvent@@'),
    ('?OnTrigger@eventobject_talonclaw_ambush@@', '?Eligible@TalonclawAmbushEvent@@'),
    ('?OnTrigger@eventobject_talonclaw_ambush@@', '?Dispatch@TalonclawAmbushEvent@@'),
    ('?OnUpdate@player_talonclaw_ambush_event@@', '?Update@TalonclawAmbushEvent@@'),
    ('?OnBeforeLogout@player_talonclaw_ambush_event@@', '?Clear@TalonclawAmbushEvent@@'),
    ('?Eligible@TalonclawAmbushEvent@@', '?Current@TalonclawAmbushEvent@@'),
    ('?Eligible@TalonclawAmbushEvent@@', '?GetConditionsForSmartEvent@ConditionMgr@@'),
    ('?Eligible@TalonclawAmbushEvent@@', '?IsObjectMeetingSmartEventConditions@ConditionMgr@@'),
    ('?Update@TalonclawAmbushEvent@@', '?HasAura@Unit@@'),
    ('?Update@TalonclawAmbushEvent@@', '?HasSceneStatus@Player@@'),
    ('?Clear@TalonclawAmbushEvent@@', '?RemoveAurasDueToSpell@Unit@@'),
    ('?Dispatch@TalonclawAmbushEvent@@', '?ProcessEventsFor@SmartScript@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing deployed lifecycle path: {caller} -> {callee}'

def constants(prefix):
    return {operand.imm for entry in instructions(prefix) for operand in entry.operands if operand.type == CS_OP_IMM}

assert {1511, 973, 40385}.issubset(constants('?Current@TalonclawAmbushEvent@@')), 'Wrong player/quest/scenario scope'
assert {10, 11, 198883, 197615, 1145}.issubset(constants('?Update@TalonclawAmbushEvent@@')), 'Wrong stage, self aura, or native scene acknowledgment'
assert {198883, 197615}.issubset(constants('?Clear@TalonclawAmbushEvent@@')), 'Owned cleanup missing one effect'
assert 500 in constants('?Update@TalonclawAmbushEvent@@'), 'Failed-cast/in-place poll is not bounded'
dispatch = [(entry.mnemonic, entry.op_str) for entry in instructions('?Dispatch@TalonclawAmbushEvent@@')]
assert ('xor', 'esi, 1') in dispatch and ('lea', 'edx, [rsi*2 + 0x59]') in dispatch, 'Native true->89/false->91 event calculation changed'
assert b'eventobject_talonclaw_ambush' in native['binary'] and b'player_talonclaw_ambush_event' in native['binary'], 'One controller registration absent'
print('PASS: deployed opt-in EO admission/entry and player update/logout hooks reach actual active-quest/source13/scenario guards, native SmartScript dispatch, self-aura/scene success checks,500ms recovery and owned cleanup; native scene packet and prior combat paths remain linked')
