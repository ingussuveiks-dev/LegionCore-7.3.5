"""Inspect Farthing's actual selected SmartAI wrapper and final-damage policy."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_prustaga_damage_release.py')))
native = previous['native']
instructions = previous['instructions']
protect = '?Protect@XalatathFarthing@@'
wrapper = '?DamageTakenAfterScaling@npc_xalatath_farthing@@'
factory = '?GetAI@?$GenericCreatureScript@Unpc_xalatath_farthing@@@@'
body = instructions(protect)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {101148, 1539, 991, 7, 8, 1}.issubset(values), 'Farthing policy scope missing'
assert native['symbol'](protect) in native['calls'](wrapper), 'Selected final-damage override omits policy'
for prefix in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@', '?GetHealth@Unit@@QEBA_KPEAV1@@', '?SetHealth@Unit@@'):
    assert native['symbol'](prefix) in native['calls'](protect), f'Missing native policy dependency: {prefix}'
health = native['symbol']('?GetHealth@Unit@@QEBA_KPEAV1@@')
position = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == health)
assert body[position + 1].mnemonic == 'cmp' and body[position + 1].op_str == 'rbx, rax'
assert body[position + 2].mnemonic == 'jb', 'Nonlethal final damage is not preserved'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rsi], r14d' for entry in body[position + 3:position + 6]), 'Final lethal damage is not cleared'
assert any(entry.mnemonic == 'xor' and entry.op_str == 'r14d, r14d' for entry in body[position + 3:position + 6])
for value, branch in ((7, 'jb'), (8, 'ja')):
    gate = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == value)
    assert body[gate + 1].mnemonic == branch, 'Native stage bounds changed'
assert native['symbol']('??0SmartAI@@') in native['calls'](factory), 'Factory omits SmartAI construction'
tables = []
for entry in instructions(factory):
    for operand in entry.operands:
        if entry.mnemonic != 'lea' or operand.type != CS_OP_MEM or operand.mem.base != X86_REG_RIP:
            continue
        address = entry.address + entry.size + operand.mem.disp
        offset = native['offset'](address)
        if offset + 0xb0 <= len(native['binary']) and struct.unpack_from('<Q', native['binary'], offset + 0xa8)[0] == native['symbol'](wrapper):
            tables.append(offset)
assert len(tables) == 1, 'Factory does not install the actual final-damage override'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0xa0)[0] == native['symbol']('?DamageTaken@SmartAI@@'), 'Native pre-scaling damaged event was replaced'
assert b'npc_xalatath_farthing' in native['binary']
print('PASS: canonical Farthing factory installs native SmartAI pre-damage and scoped101148/1539/991/7-8 post-scaled killing-blow protection; actual target-relative health comparison/zeroing and existing current/suspended-AI dispatch inspected')
