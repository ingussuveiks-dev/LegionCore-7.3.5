"""Inspect the selected Stromkar SmartAI vtable and native ingress gates."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_xalatath_opening_release.py')))
native = previous['native']
instructions = previous['instructions']
gate = '?Denied@StromkarPickup@@'
gossip = '?GossipHello@AI@go_stromkar_pickup@@'
create = '?GetAI@go_stromkar_pickup@@'
mapping = (native['args'].release / 'worldserver.map').read_text(errors='replace')
tables = re.findall(r'\s+\w+:\w+\s+\?\?_7AI@go_stromkar_pickup@@6B@\s+([0-9a-fA-F]{16})\s+', mapping)
assert len(tables) == 1, 'Missing selected Stromkar AI vtable'
table = int(tables[0], 16)
slots = struct.unpack_from('<22Q', native['binary'], native['offset'](table))
for index, prefix in {1: '?UpdateAI@SmartGameObjectAI@@', 3: '?InitializeAI@SmartGameObjectAI@@',
                      4: '?Reset@SmartGameObjectAI@@', 8: '?GossipUse@SmartGameObjectAI@@',
                      9: gossip, 20: '?OnStateChanged@SmartGameObjectAI@@', 21: '?EventInform@SmartGameObjectAI@@'}.items():
    assert slots[index] == native['symbol'](prefix), f'Wrong Stromkar AI slot{index}'
assert any(row.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP and row.address + row.size + operand.mem.disp == table for row in instructions(create) for operand in row.operands), 'Factory does not construct selected vtable'
assert native['symbol'](gate) in native['calls'](gossip), 'Report-use AI bypasses admission'
assert native['symbol'](gate) in native['calls']('?OnGossipHello@go_stromkar_pickup@@'), 'Normal/open-lock script bypasses admission'
assert native['symbol']('?GossipHello@SmartGameObjectAI@@') in native['calls'](gossip), 'Eligible gossip loses native actions'
values = {operand.imm for row in instructions(gate) for operand in row.operands if operand.type == CS_OP_IMM}
assert {247877, 41105, 1539, 1037}.issubset(values), 'Wrong opening object/quest/map/scenario'
decoded = instructions(gate)
branches = [branch for compare, branch in zip(decoded, decoded[1:]) if compare.mnemonic == 'cmp' and len(compare.operands) == 2 and compare.operands[1].type == CS_OP_IMM and compare.operands[1].imm == 9 and branch.mnemonic == 'je' and branch.operands[0].type == CS_OP_IMM]
assert len(branches) == 1, 'Missing exact9 allowed branch'
by_address = {row.address: index for index, row in enumerate(decoded)}
branch = branches[0]
accept = decoded[by_address[branch.operands[0].imm]:by_address[branch.operands[0].imm] + 8]
reject = decoded[by_address[branch.address + branch.size]:by_address[branch.address + branch.size] + 8]
assert any(row.mnemonic == 'xor' and row.op_str == 'al, al' for row in accept) and any(row.mnemonic == 'ret' for row in accept), 'Exact9 does not return allowed'
assert any(row.mnemonic == 'mov' and row.op_str == 'al, 1' for row in reject) and any(row.mnemonic == 'ret' for row in reject), 'Non9 fallthrough does not return denied'
assert b'go_stromkar_pickup' in native['binary']
print('PASS: selected Stromkar vtable retains native lifecycle/state actions and overrides report-use slot9 with the same1037/exact9 opening gate as normal/open-lock script')
