"""Inspect native jump arrival and its fresh source17 cast admission."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_execute_target_release.py')))
native = previous['native']
instructions = previous['previous']['instructions']
check = '?CheckCast@Spell@@'
lookup = native['symbol']('?GetConditionsForNotGroupedEntry@ConditionMgr@@')
body = instructions(check)
lookup_at = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == lookup]
assert len(lookup_at) == 1, 'Reinspect native source17 lookup'
arguments = body[max(0, lookup_at[0] - 12):lookup_at[0]]
assert any(entry.mnemonic == 'mov' and entry.op_str == 'r8d, 0x11' for entry in arguments), 'Cast conditions do not select source17'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'r9d, dword ptr [r9]' for entry in arguments), 'Cast conditions are not keyed by the actual spell ID'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@') in native['calls'](check), 'Missing native cast condition evaluation'
assert native['symbol'](check) in native['calls']('?prepare@Spell@@'), 'Preparation bypasses cast admission'
casts = {address for name, address in native['symbols'].items() if name.startswith('?CastSpell@Unit@@')}
arrival = '?Finalize@EffectMovementGenerator@@'
assert casts & native['calls'](arrival), 'Inlined effect arrival no longer casts through Unit'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'r9b, 1' for entry in instructions(arrival)), 'Arrival does not retain the triggered cast argument'
jumps = {address for name, address in native['symbols'].items() if name.startswith('?MoveJump@MotionMaster@@')}
assert jumps & native['calls']('?EffectJumpDest@Spell@@'), 'Jump destination no longer schedules native movement'
print('PASS: canonical jump effect reaches MotionMaster and EffectMovementGenerator arrival reaches Unit::CastSpell; actual Spell::prepare/CheckCast consumes source17 with native spell ID and conditions. SQL, trigger arguments and deferred-context matrix are separate native checks, not client traversal')
