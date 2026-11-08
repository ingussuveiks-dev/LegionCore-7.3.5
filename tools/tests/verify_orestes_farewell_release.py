"""Verify the deployed native timer/phase/credit/death paths used by Orestes."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RCX

previous = runpy.run_path(str(Path(__file__).with_name('verify_conversation_farewell_release.py')))
native = previous['native']
instructions = previous['instructions']
action = instructions('?ProcessAction@SmartScript@@')
maximum = next(index for index, entry in enumerate(action) if entry.mnemonic == 'cmp' and any(operand.type == CS_OP_IMM and operand.imm == 223 for operand in entry.operands))
assert action[maximum - 1].mnemonic == 'dec', 'Reinspect one-based native action switch'
tables = [operand.mem for entry in action[maximum + 1:maximum + 10] for operand in entry.operands if operand.type == CS_OP_MEM and operand.mem.index and operand.mem.disp > 0]
assert len(tables) == 2 and tables[0].scale == 1 and tables[1].scale == 4, 'Reinspect native compressed action switch'


def action_case(number):
    index = native['binary'][native['offset'](native['base'] + tables[0].disp + number - 1)]
    return native['base'] + struct.unpack_from('<I', native['binary'], native['offset'](native['base'] + tables[1].disp + index * 4))[0]


phase_address = action_case(22)
phase = [entry for entry in action if phase_address <= entry.address < phase_address + 40]
assert any(entry.mnemonic == 'lea' and any(operand.type == CS_OP_MEM and operand.mem.disp == 0x58 for operand in entry.operands) for entry in phase), 'Phase does not read action parameter1'
# Linked-cast state enlarged SmartScript. Bind the member to native DecPhase's
# actual read/modify/write, then require action22 and both masks to use it.
decrement = instructions('?DecPhase@SmartScript@@')
reads = [entry for entry in decrement if entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_REG and entry.operands[1].type == CS_OP_MEM and entry.operands[1].size == 4 and entry.operands[1].mem.base == X86_REG_RCX]
writes = [entry for entry in decrement if entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].size == 4 and entry.operands[0].mem.base == X86_REG_RCX and entry.operands[1].type == CS_OP_REG]
assert len(reads) == len(writes) == 1
phase_member = reads[0].operands[1].mem.disp
assert phase_member > 0 and writes[0].operands[0].mem.disp == phase_member and writes[0].operands[1].reg == reads[0].operands[0].reg
assert sum(entry.mnemonic == 'sub' for entry in decrement) == 2 and any(entry.mnemonic == 'cmovae' for entry in decrement), 'Native bounded phase decrement changed'
assert any(entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].size == 4 and entry.operands[0].mem.disp == phase_member and entry.operands[1].type == CS_OP_REG for entry in phase), 'Actual phase state write absent'
for prefix in ['?UpdateTimer@SmartScript@@', '?ProcessEvent@SmartScript@@']:
    body = instructions(prefix)
    assert any(entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_REG and entry.operands[1].type == CS_OP_MEM and entry.operands[1].size == 4 and entry.operands[1].mem.disp == phase_member for entry in body[:65]), f'Phase not consumed by {prefix}'
    assert any(entry.mnemonic == 'bt' for entry in body[:65]), f'Phase mask test absent in {prefix}'
for caller, callee in [
    ('?OnUpdate@SmartScript@@', '?UpdateTimer@SmartScript@@'),
    ('?UpdateTimer@SmartScript@@', '?ProcessEvent@SmartScript@@'),
    ('?ProcessAction@SmartScript@@', '?UpdateAchievementCriteria@Player@@'),
    ('?ProcessAction@SmartScript@@', '?Kill@Unit@@'),
    ('?ProcessAction@SmartScript@@', '?UseDoorOrButton@GameObject@@'),
    ('?IsWithinDistInMap@WorldObject@@', '?InSamePhase@WorldObject@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing native bridge {caller} -> {callee}'
assert len({action_case(number) for number in [9, 22, 37, 205]}) == 4, 'Door/phase/credit/death actions alias'
print('PASS: deployed action22 writes the actual phase consumed by timer/event bit gates; native timer dispatch, Player criteria, death and phase-aware proximity remain linked. SQL/native hosts do not prove client traversal')
