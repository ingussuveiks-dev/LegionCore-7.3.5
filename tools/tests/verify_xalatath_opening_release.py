"""Inspect the selected Xal'atath SmartAI vtable and native ingress gates."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_report_use_release.py')))
native = previous['native']
instructions = previous['instructions']
gate = '?Denied@XalatathPickup@@'
gossip = '?GossipHello@AI@go_xalatath_pickup@@'
create = '?GetAI@go_xalatath_pickup@@'
mapping = (native['args'].release / 'worldserver.map').read_text(errors='replace')
tables = re.findall(r'\s+\w+:\w+\s+\?\?_7AI@go_xalatath_pickup@@6B@\s+([0-9a-fA-F]{16})\s+', mapping)
assert len(tables) == 1, 'Missing selected Xalatath AI vtable'
table = int(tables[0], 16)
slots = struct.unpack_from('<22Q', native['binary'], native['offset'](table))
for index, prefix in {1: '?UpdateAI@SmartGameObjectAI@@', 3: '?InitializeAI@SmartGameObjectAI@@',
                      4: '?Reset@SmartGameObjectAI@@', 8: '?GossipUse@SmartGameObjectAI@@',
                      9: gossip, 20: '?OnStateChanged@SmartGameObjectAI@@', 21: '?EventInform@SmartGameObjectAI@@'}.items():
    assert slots[index] == native['symbol'](prefix), f'Wrong Xalatath AI slot{index}'
assert any(row.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP and
           row.address + row.size + operand.mem.disp == table for row in instructions(create) for operand in row.operands), 'Factory does not construct the inspected vtable'
assert native['symbol'](gate) in native['calls'](gossip), 'Report-use AI bypasses admission'
assert native['symbol'](gate) in native['calls']('?OnGossipHello@go_xalatath_pickup@@'), 'Ordinary/open-lock script bypasses admission'
assert native['symbol']('?GossipHello@SmartGameObjectAI@@') in native['calls'](gossip), 'Eligible gossip loses native actions'
values = {operand.imm for row in instructions(gate) for operand in row.operands if operand.type == CS_OP_IMM}
assert {247690, 40710, 1539, 991}.issubset(values), 'Wrong opening object/quest/map/scenario'
bounds = {}
decoded = instructions(gate)
for compare, branch in zip(decoded, decoded[1:]):
    if compare.mnemonic != 'cmp' or len(compare.operands) != 2 or compare.operands[1].type != CS_OP_IMM:
        continue
    expected = {8: 'jb', 9: 'jbe'}.get(compare.operands[1].imm)
    if expected and branch.mnemonic == expected and branch.operands[0].type == CS_OP_IMM:
        bounds[compare.operands[1].imm] = branch.operands[0].imm
assert bounds.keys() == {8, 9}, 'Missing unsigned exact8/9 opening bounds'
by_address = {row.address: index for index, row in enumerate(decoded)}
reject = decoded[by_address[bounds[8]]:by_address[bounds[8]] + 8]
accept = decoded[by_address[bounds[9]]:by_address[bounds[9]] + 8]
assert any(row.mnemonic == 'mov' and row.op_str == 'al, 1' for row in reject) and any(row.mnemonic == 'ret' for row in reject), 'Early stage does not return denied'
assert accept[0].mnemonic == 'xor' and accept[0].op_str == 'al, al' and any(row.mnemonic == 'ret' for row in accept), 'Valid upper bound does not return allowed'
assert any(row.mnemonic == 'jbe' and row.operands[0].type == CS_OP_IMM and row.operands[0].imm == bounds[9] and row.address + row.size == bounds[8] for row in decoded), 'Late stage does not fall through to rejection'
assert b'go_xalatath_pickup' in native['binary']
print('PASS: actual selected Xalatath vtable retains native lifecycle/state actions, overrides report-use slot9 with the same opening gate as normal/open-lock script, exact991/8-9 and native eligible gossip')
