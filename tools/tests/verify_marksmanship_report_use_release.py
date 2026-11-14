"""Verify the real selected vtable, not just a script registration string."""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_MEM, CS_OP_REG, CS_OP_IMM
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
def report_gossip_dispatch(rows, ai_address, resolve):
    """Prove AI()->vtable[9](AI, player, false), preserving scheduled values."""
    def family(name):
        for full, aliases in {'rax': ('eax','ax','al','ah'), 'rbx': ('ebx','bx','bl','bh'),
                              'rcx': ('ecx','cx','cl','ch'), 'rdx': ('edx','dx','dl','dh'),
                              'rsi': ('esi','si','sil'), 'rdi': ('edi','di','dil'),
                              'rbp': ('ebp','bp','bpl'), 'rsp': ('esp','sp','spl')}.items():
            if name == full or name in aliases:
                return full
        return name.rstrip('dwb') if name.startswith('r') and name[1:2].isdigit() else name
    found = []
    for index, load in enumerate(rows):
        if index < 2 or load.mnemonic != 'mov' or len(load.operands) != 2:
            continue
        target, slot = load.operands
        if target.type != CS_OP_REG or slot.type != CS_OP_MEM or slot.size != 8 or slot.mem.disp != 9 * 8 or slot.mem.index:
            continue
        get_ai, vptr = rows[index-2:index]
        if not (get_ai.mnemonic == 'call' and get_ai.operands[0].type == CS_OP_IMM and
                resolve(get_ai.operands[0].imm) == ai_address and vptr.mnemonic == 'mov' and
                len(vptr.operands) == 2 and vptr.operands[0].type == CS_OP_REG and
                vptr.operands[0].reg == slot.mem.base and vptr.operands[1].type == CS_OP_MEM and
                vptr.operands[1].size == 8 and vptr.reg_name(vptr.operands[1].mem.base) == 'rax' and
                not vptr.operands[1].mem.index and vptr.operands[1].mem.disp == 0):
            continue
        target_family = family(load.reg_name(target.reg))
        this_ok = false_ok = False
        for call_index in range(index+1, min(len(rows), index+9)):
            row = rows[call_index]
            if row.mnemonic == 'call':
                if (row.operands[0].type == CS_OP_REG and
                        family(row.reg_name(row.operands[0].reg)) == target_family and this_ok and false_ok):
                    found.append((index, call_index))
                break
            if row.mnemonic not in ('mov', 'xor', 'lea', 'nop'):
                break
            writes = {family(row.reg_name(reg)) for reg in row.regs_access()[1]}
            if target_family in writes or 'rax' in writes:
                break
            if 'rcx' in writes:
                this_ok = row.mnemonic == 'mov' and row.op_str == 'rcx, rax'
            if 'r8' in writes:
                false_ok = (row.mnemonic == 'xor' and len(row.operands) == 2 and
                            all(o.type == CS_OP_REG and o.size >= 4 for o in row.operands) and
                            row.operands[0].reg == row.operands[1].reg)
    return found

assert len(report_gossip_dispatch(report, native['symbol']('?AI@GameObject@@'), native['resolve'])) == 1, 'Native report-use must dispatch the checked slot9 with the real AI and false flag'
assert b'go_marksmanship_pickup' in native['binary']
print('PASS: canonical factory constructs the checked Thasdorah vtable; actual report-use slot9 runs scoped admission and native gossip while all inspected SmartAI lifecycle slots stay inherited')
