"""Check the linked final Vigfus death callback, owner bridge and selected vtable."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
stamp = re.search(r'Timestamp is ([0-9a-fA-F]+)', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp.group(1), 16) == struct.unpack_from('<I', native['binary'], native['pe'] + 8)[0], 'Canonical PE/map link timestamps differ'
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address - native['base']) - 1
    begin, end, unwind = native['functions'][index]
    assert begin <= address - native['base'] < end, f'Missing runtime bounds: {prefix}'
    origin = native['root'](begin, unwind)
    for tail_begin, tail_end, tail_unwind in native['functions'][index + 1:]:
        if tail_begin != end or native['root'](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    start = native['offset'](address)
    return list(decoder.disasm(native['binary'][start:start + native['base'] + end - address], address))


callback = '?JustDied@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
factory = '?GetAI@npc_vigfus_bladewind_last@@'
owner = '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@'
body = instructions(callback)
targets = native['calls'](callback)
for dependency in (owner, '?UpdateAchievementCriteria@Player@@', '?CreateConversation@Unit@@', '?Talk@CreatureAI@@QEAAXE'):
    assert native['symbol'](dependency) in targets, f'Missing native death dependency: {dependency}'
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {46987, 848, 92, 2} <= values, 'Native progression/conversation/death text changed'
owner_call = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                  and native['resolve'](entry.operands[0].imm) == native['symbol'](owner))
assert any(entry.mnemonic == 'test' and entry.op_str == 'rax, rax' for entry in body[owner_call + 1:owner_call + 5]), 'Missing unresolved-owner guard'
killer_registers = {'rdx'}
for entry in body[:owner_call]:
    if entry.mnemonic == 'mov' and len(entry.operands) == 2 and all(operand.type == CS_OP_REG for operand in entry.operands):
        if entry.reg_name(entry.operands[1].reg) in killer_registers:
            killer_registers.add(entry.reg_name(entry.operands[0].reg))
assert any(entry.mnemonic == 'test' and any(entry.op_str == f'{register}, {register}' for register in killer_registers)
           and body[index + 1].mnemonic == 'je' and body[index + 1].operands[0].imm > body[owner_call].address
           for index, entry in enumerate(body[:owner_call])), 'Missing null-killer branch around owner resolution'
tables = []
for entry in instructions(factory):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            table = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<80Q', native['binary'], table)
            if native['symbol'](callback) in slots:
                tables.append((table, slots.index(native['symbol'](callback)) * 8))
assert len(tables) == 1, 'Factory does not select the repaired final death callback'
assert tables[0][1] == 0x130, f'Unexpected actual JustDied slot: {tables[0][1]:x}'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == tables[0][1]
           for entry in instructions('?Kill@Unit@@')), 'Native death dispatcher does not call the selected JustDied slot'
scenario_targets = {native['resolve'](address) for name, address in native['symbols'].items()
                    if name.startswith('?UpdateAchievementCriteria@Scenario@@')}
assert scenario_targets & native['calls']('?UpdateAchievementCriteria@Player@@'), 'Native player event no longer reaches scenario progression'
assert b'npc_vigfus_bladewind_last' in native['binary'], 'Final boss registration missing'
print('PASS: canonical Vigfus factory selects repaired death callback; native owner resolution/null guards and original 46987/848/Talk2 retained. Not a client combat or visual traversal.')
