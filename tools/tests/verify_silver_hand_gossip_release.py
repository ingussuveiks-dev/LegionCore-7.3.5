"""Verify actual first-Travard packet selection, admission and one-shot path dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_MEM
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
                         ('rsi, rsi', 'je'), ('byte ptr [rsi], 0', 'jne'),
                         ('dword ptr [rsi + 4], 0', 'jne'), ('dword ptr [rsi + 8], 0', 'jne'),
                         ('byte ptr [rsi + 0x50], 0', 'jne')):
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
def verify_script_path(decoded, path_id, consume):
    waypoints, = sites(decoded, '?instance@WaypointMgr@@')
    lookup, = sites(decoded, '?GetPathScript@WaypointMgr@@')
    assert waypoints < lookup < consume
    assert any(entry.op_str == f'edx, {path_id:#x}' for entry in decoded[waypoints:lookup])
    assert decoded[lookup + 1].op_str == 'rax, rax' and decoded[lookup + 2].mnemonic == 'je'
    warning = decoded[lookup + 2].operands[0].imm
    assert not reaches(decoded, warning, decoded[consume].address)
    assert decoded[lookup + 3].op_str == 'rcx, qword ptr [rax + 8]'
    assert decoded[lookup + 4].op_str == 'qword ptr [rax], rcx'
    assert decoded[lookup + 5].mnemonic == 'je' and decoded[lookup + 5].operands[0].imm == warning
    address = native['symbol']('?GetPathScript@WaypointMgr@@')
    end = min(value for value in native['symbols'].values() if value > address)
    assert 0 < end - address < 512
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    leaf = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](end)], address))
    assert leaf[0].op_str == 'qword ptr [rcx + 0x50], 0'
    assert leaf[1].op_str == 'r9, rcx' and leaf[2].op_str == 'r8d, edx'
    assert leaf[3].mnemonic == 'je'
    deny = leaf[3].operands[0].imm
    terminal = next(index for index, entry in enumerate(leaf) if entry.address == deny)
    assert leaf[terminal].mnemonic == 'xor' and leaf[terminal].op_str == 'eax, eax'
    assert leaf[terminal + 1].mnemonic == 'ret'
    assert all(any(entry.op_str == operands for entry in leaf) for operands in
               ('rdx, qword ptr [r9 + 0x48]', 'rcx, qword ptr [r9 + 0x70]',
                'rcx, qword ptr [r9 + 0x58]', 'r8d, dword ptr [rax + 0x10]'))
    sentinel = next(index for index, entry in enumerate(leaf) if entry.op_str == 'rax, rdx' and entry.mnemonic == 'cmp')
    assert leaf[sentinel + 1].mnemonic == 'je'
    bucket = next(index for index, entry in enumerate(leaf) if entry.address == leaf[sentinel + 1].operands[0].imm)
    assert leaf[bucket].mnemonic == 'xor' and leaf[bucket].op_str == 'eax, eax'
    assert leaf[bucket + 1].op_str == 'rax, rax' and leaf[bucket + 2].op_str == 'rax, rdx'
    assert leaf[bucket + 2].mnemonic == 'cmove'
    assert leaf[bucket + 3].op_str == 'rax, rdx' and leaf[bucket + 4].operands[0].imm == deny
    assert leaf[bucket + 4].mnemonic == 'je'
    assert leaf[bucket + 5].op_str == 'rax, 0x18' and leaf[bucket + 6].mnemonic == 'ret'


verify_script_path(body, 10993804, remove)
assert any(entry.op_str == 'edx, 0xa7c08c' for entry in body[move - 5:move])
assert any(entry.op_str == 'r8d, r8d' and entry.mnemonic == 'xor' for entry in body[move - 5:move])
assert not any(name.startswith('?OnGossipSelect@npc_travar_first@@') for name in native['symbols']), 'Legacy unvalidated CreatureScript override remains'
for method in ('?SetUInt32Value@Object@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@', '?AddItem@Player@@'):
    assert not sites(body, method)
table, = previous['selected']
assert struct.unpack_from('<Q', native['binary'], native['offset'](table) + 0xc8)[0] == native['symbol'](prefix)
packet = instructions('?HandleGossipSelectOption@WorldSession@@')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xc8 for entry in packet), 'Native packet does not dispatch checked AI selection slot'
print('PASS: native packet dispatches selected first-Travard AI slot0xc8, not old unvalidated CreatureScript; exact19580/0 and stored menu/item plus live Holy1092/step1/ready gossip/firstwp/5yd gate before path10993804. Missing/empty native script-path branches retain retry and diagnostics; only gossip bit removed and own menus cleared before one path attempt. No direct credit/item or actual movement/client proof.')
