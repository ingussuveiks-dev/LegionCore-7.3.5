"""Verify installed bounded Ashbringer Incinerator ground-target emission."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_R14, X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


scope = 'npc_bs_doomguard_incineratorAI@npc_bs_doomguard_incinerator@@'
factory = instructions('?GetAI@npc_bs_doomguard_incinerator@@')
methods = {method: '?' + method + '@' + scope for method in ('UpdateAI', 'IsSummonedBy', 'MovementInform')}
tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if all(slots[index // 8] == native['symbol'](methods[method]) for method, index in (('UpdateAI', 0x18), ('IsSummonedBy', 0x148), ('MovementInform', 0x1a8))):
            tables.append(slots)
assert len(tables) == 1
assert any(entry.op_str == 'ecx, 0x60' for entry in factory)
body = instructions(methods['UpdateAI'])
execute, = sites(body, '?ExecuteEvent@EventMap@@')
cast, = sites(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@UPosition@@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@@Z')
cancel, = sites(body, '?CancelEvent@EventMap@@')
schedule, = sites(body, '?ScheduleEvent@EventMap@@QEAAXII')
assert execute < cast < cancel < schedule
assert any(entry.op_str == 'dword ptr [rcx + 0x38], edx' and entry.mnemonic == 'add' for entry in body[:execute])
guards = [index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'al, 0x24']
assert len(guards) == 2 and execute < guards[0] < cast < guards[1] < cancel
assert body[guards[0] - 1].op_str == 'eax, byte ptr [rdi + 0x58]'
reads = [index for index, entry in enumerate(body[:cast]) if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_R14]
assert len(reads) == 4 and guards[0] < min(reads) <= max(reads) < cast
for guard, limit in ((guards[0], cast), (guards[1], schedule)):
    assert body[guard + 1].mnemonic == 'jae' and body[guard + 1].operands[0].imm > body[limit].address
assert body[cast + 1].mnemonic == 'inc' and body[cast + 1].op_str == 'byte ptr [rdi + 0x58]'
batch, = [index for index, entry in enumerate(body) if entry.op_str == 'bpl, 3' and entry.mnemonic == 'cmp']
assert body[batch + 1].mnemonic == 'jb' and body[batch + 1].operands[0].imm == body[guards[0] - 1].address
assert all(any(entry.op_str == operands for entry in body[:cast]) for operands in ('r9b, 1', 'r8d, 0x2e8ca', 'rdx, [rsp + 0x40]'))
assert body[schedule - 1].op_str == 'r8d, 0x3e8' and body[schedule - 2].op_str == 'rcx, [rdi + 0x38]'
target, = [entry for entry in body[:cast] if entry.mnemonic == 'lea' and entry.op_str.startswith('r14, [rip +')]
address = target.address + target.size + target.operands[1].mem.disp
header = (Path(__file__).parents[2] / 'src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h').read_text(encoding='utf-8-sig')
table = header.split('Position const flameTarget[36] =', 1)[1].split('};', 1)[0]
positions = [list(map(float, re.findall(r'-?\d+(?:\.\d+)?(?=f)', row))) for row in re.findall(r'\{([^{}]+)\}', table)]
assert len(positions) == 36 and all(len(position) == 3 for position in positions)
initializer = instructions('??__EflameTarget@@YAXXZ')
writes = {}
for entry in initializer:
    if entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.base == X86_REG_RIP and entry.operands[1].type == CS_OP_IMM:
        destination = entry.address + entry.size + entry.operands[0].mem.disp
        if address <= destination < address + 36 * 16:
            assert destination not in writes
            writes[destination] = struct.pack('<I', entry.operands[1].imm & 0xffffffff)
assert len(writes) == 35 * 3
for target_index, position in enumerate(positions):
    for coordinate, value in enumerate(position):
        destination = address + target_index * 16 + coordinate * 4
        actual = writes[destination] if destination in writes else native['binary'][native['offset'](destination):native['offset'](destination) + 4]
        assert actual == struct.pack('<f', value)
registrations = re.findall(r'\?flameTarget\$initializer\$@@3P6AXXZEA\s+([0-9a-fA-F]{16})\s+scripts:broken_shore.obj', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert len(registrations) == 1
assert struct.unpack_from('<Q', native['binary'], native['offset'](int(registrations[0], 16)))[0] == native['symbol']('??__EflameTarget@@YAXXZ')
summon = instructions(methods['IsSummonedBy'])
assert any(entry.op_str == 'byte ptr [rcx + 0x58], 0' for entry in summon)
assert len(sites(summon, '?ScheduleEvent@EventMap@@QEAAXII')) == 1
arrival = instructions(methods['MovementInform'])
reset, = sites(arrival, '?Reset@EventMap@@')
visible, = sites(arrival, '?SetVisible@WorldObject@@')
assert reset < visible and arrival[visible - 1].op_str == 'edx, edx'
assert arrival[0].op_str == 'edx, 8' and any(entry.op_str == 'r8d, 1' for entry in arrival[:reset])
print('PASS selected95962 factory60 binds actual emission/summon/arrival. Native EventMap38/target byte58, unsigned<36 guard before target read/cast, three-per-tick backedge, no reschedule at exhaustion, original190666 triggered Position call/1000ms and all36 installed16-byte positions retained. Arrival POINT8/id1 still resets/hides. No owner/quest credit, client VFX or actual motion proof.')
