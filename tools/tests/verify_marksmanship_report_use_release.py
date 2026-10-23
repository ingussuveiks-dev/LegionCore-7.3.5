"""Verify the real selected vtable, not just a script registration string."""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_pickup_admission_release.py')))
native = previous['native']
instructions = previous['instructions']
gossip = '?GossipHello@AI@go_marksmanship_pickup@@'
create = '?GetAI@go_marksmanship_pickup@@'
mapping = (native['args'].release / 'worldserver.map').read_text(errors='replace')
tables = re.findall(r'\s+\w+:\w+\s+\?\?_7AI@go_marksmanship_pickup@@6B@\s+([0-9a-fA-F]{16})\s+', mapping)
assert len(tables) == 1, 'Missing or ambiguous selected AI data vtable'
table = int(tables[0], 16)
start = native['offset'](table)
slots = struct.unpack_from('<22Q', native['binary'], start)
for index, prefix in {
    1: '?UpdateAI@SmartGameObjectAI@@',
    3: '?InitializeAI@SmartGameObjectAI@@',
    4: '?Reset@SmartGameObjectAI@@',
    8: '?GossipUse@SmartGameObjectAI@@',
    9: gossip,
    20: '?OnStateChanged@SmartGameObjectAI@@',
    21: '?EventInform@SmartGameObjectAI@@',
}.items():
    assert slots[index] == native['symbol'](prefix), f'Wrong selected SmartAI vtable slot{index}'
assert any(instruction.mnemonic == 'lea' and operand.type == CS_OP_MEM and
           operand.mem.base == X86_REG_RIP and instruction.address + instruction.size + operand.mem.disp == table
           for instruction in instructions(create) for operand in instruction.operands), 'GetAI does not construct the checked derived vtable'
assert native['symbol']('?Use@ArtifactAcquisition@@') in native['calls'](gossip), 'Direct AI gossip omits scoped admission'
assert native['symbol']('?GossipHello@SmartGameObjectAI@@') in native['calls'](gossip), 'Valid gossip loses native SmartAI dispatch'
assert native['symbol']('?GetGameObjectAI@ScriptMgr@@') in native['calls']('?SelectGameObjectAI@FactorySelector@@'), 'AI selection omits the registered factory'
report = instructions('?HandleGameobjectReportUse@WorldSession@@')
assert any(load.mnemonic == 'mov' and len(load.operands) == 2 and load.operands[0].type == CS_OP_REG and
           load.operands[1].type == CS_OP_MEM and load.operands[1].mem.disp == 9 * 8 and
           any(call.mnemonic == 'call' and len(call.operands) == 1 and call.operands[0].type == CS_OP_REG and
               call.operands[0].reg == load.operands[0].reg for call in report[index + 1:index + 4])
           for index, load in enumerate(report)), 'Native report-use no longer loads/calls the checked GossipHello slot'
assert b'go_marksmanship_pickup' in native['binary']
print('PASS: canonical factory constructs the checked Thasdorah vtable; actual report-use slot9 runs scoped admission and native gossip while all inspected SmartAI lifecycle slots stay inherited')
