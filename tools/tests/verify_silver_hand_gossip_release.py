"""Verify actual first-Travard packet selection, admission and one-shot path dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_entrance_release.py')))
native, instructions = previous['native'], previous['instructions']
sites, reaches = previous['sites'], previous['reaches']
prefix = '?sGossipSelect@npc_travar_firstAI@npc_travar_first@@'
body = instructions(prefix)
move, = sites(body, '?MovePath@MotionMaster@@')
remove, = sites(body, '?RemoveFlag@Object@@')
clear, = sites(body, '?ClearMenus@PlayerMenu@@')
close, = sites(body, '?SendCloseGossip@PlayerMenu@@')
assert remove < clear < close < move
assert body[0].op_str == 'r8d, 0x4c7c' and body[1].mnemonic == 'jne'
assert not reaches(body, body[1].operands[0].imm, body[remove].address)


def reject(operands, branch):
    position = next(index for index, entry in enumerate(body[:remove]) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test'))
    jump = body[position + 1]
    assert jump.mnemonic == branch and not reaches(body, jump.operands[0].imm, body[remove].address)


for operands, branch in (('r9d, r9d', 'jne'), ('byte ptr [rcx + 0x5a], r9b', 'je'),
                         ('rax, rax', 'je'), ('eax, 1', 'jne'), ('eax, 0x4c7c', 'jne'),
                         ('rdi, rdi', 'je'), ('byte ptr [rdi], 0', 'jne'),
                         ('dword ptr [rdi + 4], 0', 'jne'), ('dword ptr [rdi + 8], 0', 'jne'),
                         ('byte ptr [rdi + 0x50], 0', 'jne')):
    reject(operands, branch)
for method in ('?CanArrive@SilverHandEntrance@@', '?HasFlag@Object@@', '?IsWithinDistInMap@WorldObject@@'):
    position, = sites(body, method)
    assert body[position + 1].op_str == 'al, al' and body[position + 2].mnemonic == 'je'
    assert not reaches(body, body[position + 2].operands[0].imm, body[remove].address)
has_flag, = sites(body, '?HasFlag@Object@@')
for position in (has_flag, remove):
    assert any(entry.op_str == 'edx, 0x7d' for entry in body[position - 5:position])
    assert any(entry.op_str == 'r8d, 1' for entry in body[position - 5:position])
distance, = sites(body, '?IsWithinDistInMap@WorldObject@@')
loads = [entry for entry in body[distance - 7:distance] if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP]
assert len(loads) == 1
address = loads[0].address + loads[0].size + loads[0].operands[1].mem.disp
assert struct.unpack_from('<f', native['binary'], native['offset'](address))[0] == 5.0
item, = sites(body, '?GetItem@GossipMenu@@')
assert any(entry.op_str == 'edx, edx' and entry.mnemonic == 'xor' for entry in body[item - 4:item])
step, = sites(body, '?GetCurrentStep@Scenario@@')
assert body[step + 1].op_str == 'eax, 1' and body[step + 2].mnemonic == 'jne'
waypoints, = sites(body, '?instance@WaypointMgr@@')
assert waypoints < remove and body[waypoints + 2].op_str == 'ebx, 0xa7c08c'
path_empty = next(index for index, entry in enumerate(body[:remove]) if entry.op_str == 'qword ptr [rcx], rax')
assert body[path_empty + 1].mnemonic == 'je'
warning = body[path_empty + 1].operands[0].imm
assert not reaches(body, warning, body[remove].address)
for operands in ('qword ptr [rax + 0x50], 0', 'rcx, rcx'):
    positions = [index for index, entry in enumerate(body[waypoints:remove], waypoints) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test')]
    assert len(positions) == 1
    assert body[positions[0] + 1].mnemonic == 'je' and body[positions[0] + 1].operands[0].imm == warning
sentinels = [index for index, entry in enumerate(body[waypoints:remove], waypoints) if entry.op_str == 'rcx, qword ptr [r8 + 0x48]' and entry.mnemonic == 'cmp']
assert len(sentinels) == 2
assert body[sentinels[1] + 1].mnemonic == 'je' and body[sentinels[1] + 1].operands[0].imm == warning
assert body[sentinels[0] + 1].mnemonic == 'je'
empty_bucket = next(index for index, entry in enumerate(body) if entry.address == body[sentinels[0] + 1].operands[0].imm)
assert body[empty_bucket].mnemonic == 'xor' and body[empty_bucket].op_str == 'ecx, ecx'
assert body[empty_bucket + 1].op_str == 'rcx, rcx' and body[empty_bucket + 2].operands[0].imm == warning
assert any(entry.op_str == 'edx, ebx' for entry in body[move - 5:move])
assert any(entry.op_str == 'r8d, r8d' and entry.mnemonic == 'xor' for entry in body[move - 5:move])
assert not any(name.startswith('?OnGossipSelect@npc_travar_first@@') for name in native['symbols']), 'Legacy unvalidated CreatureScript override remains'
for method in ('?SetUInt32Value@Object@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@', '?AddItem@Player@@'):
    assert not sites(body, method)
table, = previous['selected']
assert struct.unpack_from('<Q', native['binary'], native['offset'](table) + 0xc8)[0] == native['symbol'](prefix)
packet = instructions('?HandleGossipSelectOption@WorldSession@@')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xc8 for entry in packet), 'Native packet does not dispatch checked AI selection slot'
print('PASS: native packet dispatches selected first-Travard AI slot0xc8, not old unvalidated CreatureScript; exact19580/0 and stored menu/item plus live Holy1092/step1/ready gossip/firstwp/5yd gate before path10993804. Missing/empty native script-path branches retain retry and diagnostics; only gossip bit removed and own menus cleared before one path attempt. No direct credit/item or actual movement/client proof.')
