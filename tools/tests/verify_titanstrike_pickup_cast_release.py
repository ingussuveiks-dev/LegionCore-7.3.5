"""Inspect the native delayed-cast consumer and generic source17 admission."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_pickup_admission_release.py')))
native = previous['native']
instructions = previous['instructions']
check = '?CheckCast@Spell@@'
lookup = native['symbol']('?GetConditionsForNotGroupedEntry@ConditionMgr@@')
body = instructions(check)
lookup_at = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == lookup]
assert len(lookup_at) == 1, 'Reinspect actual source17 condition lookup'
arguments = body[max(0, lookup_at[0] - 12):lookup_at[0]]
assert any(entry.mnemonic == 'mov' and entry.op_str == 'r8d, 0x11' for entry in arguments), 'Native spell-condition lookup does not select source17 after its hidden list return argument'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'r9d, dword ptr [r9]' for entry in arguments), 'Native condition lookup is not keyed by the actual SpellInfo ID'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@') in native['calls'](check), 'Spell condition list is not evaluated'
assert native['symbol'](check) in native['calls']('?prepare@Spell@@'), 'Ordinary pickup preparation bypasses cast admission'
assert native['symbol']('?AddDelayedEvent@Unit@@') in native['calls']('?ProcessAction@SmartScript@@'), 'SmartAI no longer uses the Unit-owned delayed queue'
assert native['symbol']('?AddDelayedEvent@FunctionProcessor@@') in native['calls']('?AddDelayedEvent@Unit@@'), 'Unit queue bypasses the native FunctionProcessor'
print('PASS: canonical Spell::prepare calls CheckCast, whose actual source17 lookup/evaluator uses the real spell ID; source17 pickup SQL records and200ms timing are verified by separate native fixtures')
