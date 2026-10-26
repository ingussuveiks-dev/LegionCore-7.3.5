"""Verify selected Zerus point admission before helper/beam side effects."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_ashbringer_flame_targets_release.py')))
native, instructions, sites = (previous[key] for key in ('native', 'instructions', 'sites'))
scope = 'npc_bs_boss_jailer_zerusAI@npc_bs_boss_jailer_zerus@@'
factory = instructions('?GetAI@npc_bs_boss_jailer_zerus@@')
tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if all(slots[index // 8] == native['symbol']('?' + method + '@' + scope) for method, index in (('UpdateAI', 0x18), ('MovementInform', 0x1a8))):
            tables.append(slots)
assert len(tables) == 1 and any(entry.op_str == 'ecx, 0xc8' for entry in factory)
body = instructions('?MovementInform@' + scope)
assert body[0].mnemonic == 'cmp' and body[0].op_str == 'edx, 8'
assert body[1].mnemonic == 'jne' and body[-1].mnemonic == 'ret' and body[1].operands[0].imm == body[-1].address
first, = [index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'r8d, 1']
second, = [index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'r8d, 2']
assert first < second and body[first + 1].mnemonic == 'je' and body[second + 1].mnemonic == 'jne'
facing, = sites(body, '?SetFacingTo@Unit@@QEAAXM@Z')
cast, = sites(body, '?DoCast@UnitAI@@QEAAXPEAVUnit@@I_N@Z')
summon, = sites(body, '?SummonCreature@WorldObject@@QEBAPEAVTempSummon@@IAEBUPosition@@W4TempSummonType@@')
cancel, = sites(body, '?CancelEvent@EventMap@@')
schedule, = sites(body, '?ScheduleEvent@EventMap@@QEAAXII')
assert second < facing < cast < summon < cancel < schedule
assert body[second + 1].operands[0].imm > body[schedule].address
point_one, = [index for index, entry in enumerate(body) if entry.address == body[first + 1].operands[0].imm]
assert second < point_one < facing and body[point_one].mnemonic == 'movss'
assert all(any(entry.op_str == operands for entry in body[:cast]) for operands in ('r9b, 1', 'r8d, 0x2f054', 'rdx, qword ptr [rbx + 0x20]'))
assert all(any(entry.op_str == operands for entry in body[cast:summon]) for operands in ('r8d, [rdi + 1]', 'r8, 4', 'edx, 0x16631'))
array, = [entry for entry in body[cast:summon] if entry.mnemonic == 'lea' and entry.op_str.startswith('rax, [rip +')]
array_address = array.address + array.size + array.operands[1].mem.disp
map_text = (native['args'].release / 'worldserver.map').read_text(errors='replace')
addresses = re.findall(r'\?zerusSumPos@@3QBUPosition@@B\s+([0-9a-fA-F]{16})\s+scripts:broken_shore.obj', map_text)
assert len(addresses) == 1 and array_address == int(addresses[0], 16)
assert body[schedule - 1].op_str == 'r8d, 0x3e8' and body[schedule - 2].op_str == 'rcx, [rbx + 0xa0]'
assert any(entry.op_str == 'edx, 4' for entry in body[cancel:schedule])
producer = instructions('?UpdateAI@' + scope)
move, = sites(producer, '?MovePoint@MotionMaster@@QEAAXIMMM_NM@Z')
assert producer[move - 2].op_str == 'edx, esi'
assert any(entry.op_str == 'esi, 1' for entry in producer[:move]) and any(entry.op_str == 'esi, 2' for entry in producer[:move])
print('PASS selected91672 factoryc8/MovementInform1a8 rejects non-POINT8 and every ID except1/2 before facing/192596 triggered beam/91697 helper and EventMapA0. Native id+1/16-byte array index stays within zerusSumPos2/3; original1000ms EVENT4 retained. Linked flame bounds remain. No issued-request/duplicate/attempt identity or client VFX proof.')
