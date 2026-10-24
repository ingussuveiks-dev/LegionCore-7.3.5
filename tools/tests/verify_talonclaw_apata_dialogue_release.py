"""Verify distinct speech/cast opcodes and native text-timer continuation."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_orestes_farewell_release.py')))
native = previous['native']
instructions = previous['instructions']
destinations = {previous['action_case'](number) for number in range(1, 224)}
action = instructions('?ProcessAction@SmartScript@@')


def branch(number):
    address = previous['action_case'](number)
    end = min(target for target in destinations if target > address)
    return [entry for entry in action if address <= entry.address < end]


def calls(entries):
    return {native['resolve'](entry.operands[0].imm) for entry in entries
            if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM}


talk = branch(1)
cast = branch(11)
send = native['symbol']('?SendChat@CreatureTextMgr@@')
assert send in calls(talk) and send not in calls(cast), 'TALK and CAST no longer have distinct speech behavior'
cast_name = '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@'
assert native['symbol'](cast_name) in calls(cast), 'Old CAST opcode no longer requests an actual spell'
for displacement, size in [(0x120, 4), (0x124, 4), (0x138, 4), (0x13c, 1)]:
    assert any(entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM
               and entry.operands[0].mem.disp == displacement and entry.operands[0].size == size
               for entry in talk), f'TALK omits actual text-timer field {displacement:x}'
update = instructions('?OnUpdate@SmartScript@@')
timer_load = next(index for index, entry in enumerate(update) if entry.mnemonic == 'mov'
                  and entry.operands[0].type == CS_OP_REG and entry.operands[1].type == CS_OP_MEM
                  and entry.operands[1].mem.disp == 0x120)
assert update[timer_load + 1].mnemonic == 'cmp' and update[timer_load + 1].operands[0].reg == update[timer_load].operands[0].reg
assert update[timer_load + 2].mnemonic == 'jae', 'Expiry no longer retains native timer < diff boundary'
assert any(entry.mnemonic == 'mov' and any(operand.type == CS_OP_IMM and operand.imm == 52 for operand in entry.operands)
           for entry in update), 'Actual TEXT_OVER52 event missing'
for caller, callee in [('?OnUpdate@SmartScript@@', '?ProcessEventsFor@SmartScript@@'),
                       ('?ProcessEventsFor@SmartScript@@', '?ProcessEvent@SmartScript@@'),
                       ('?ProcessEvent@SmartScript@@', '?ProcessAction@SmartScript@@'),
                       ('?ProcessAction@SmartScript@@', '?StartPath@SmartAI@@')]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing native continuation {caller} -> {callee}'
print('PASS: canonical action1 publishes chat and real timer/group/talker fields; distinct action11 casts the wrong spell; actual expiry emits52 through native event/action continuation and retains StartPath')
