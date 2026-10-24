"""Verify Hati's native SmartAI event and spell-condition consumers in Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_pickup_cast_release.py')))
native = previous['native']
instructions = previous['instructions']
dispatch = '?ProcessEventsFor@SmartScript@@'
for callback, event in (('?OnSpellClick@SmartAI@@', 73), ('?SpellHit@SmartAI@@', 8)):
    assert native['symbol'](dispatch) in native['calls'](callback), f'Missing actual SmartAI callback: {callback}'
    assert any(entry.mnemonic == 'mov' and len(entry.operands) == 2 and entry.operands[0].type == CS_OP_REG and
               entry.reg_name(entry.operands[0].reg) == 'edx' and entry.operands[1].type == CS_OP_IMM and
               entry.operands[1].imm == event for entry in instructions(callback)), 'Wrong native event identity'
targets = native['calls'](dispatch)
assert native['symbol']('?IsObjectMeetingSmartEventConditions@ConditionMgr@@') in targets, 'Event admission before dispatch is missing'
assert native['symbol']('?ProcessEvent@SmartScript@@') in targets, 'Actual event dispatch is missing'
body = instructions('?ProcessEvent@SmartScript@@')
guard = next(index for index, entry in enumerate(body) if entry.mnemonic == 'test' and entry.op_str == 'byte ptr [r12 + 0x3c], 1')
assert body[guard + 1].mnemonic == 'je'
assert body[guard + 2].mnemonic == 'cmp' and body[guard + 2].op_str == 'byte ptr [r12 + 0x99], 0'
assert body[guard + 3].mnemonic == 'jne', 'Native nonrepeatable event no longer checks runOnce'
lookup = native['symbol']('?GetConditionsForSmartEvent@ConditionMgr@@')
assert any(index > guard and entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and
           native['resolve'](entry.operands[0].imm) == lookup for index, entry in enumerate(body)), 'Event conditions are not evaluated after the native repeat guard'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@') in native['calls']('?ProcessEvent@SmartScript@@')
print('PASS: actual Release SmartAI click73/hit8 dispatch, source22 consumers and native flag1/runOnce guard remain linked; generic source17 cast admission is verified separately from the installed Hati data')
