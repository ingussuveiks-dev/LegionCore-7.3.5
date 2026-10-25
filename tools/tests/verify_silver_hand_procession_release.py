"""Verify the native second-Travard waypoint recipient and consumed full GUID."""
from pathlib import Path
import runpy
import struct

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_second_gossip_release.py')))
native, instructions = previous['native'], previous['instructions']
sites, reaches = previous['sites'], previous['reaches']
movement = '?MovementInform@npc_travar_secondAI@npc_travar_second@@'
body = instructions(movement)
empty, = sites(body, '?IsEmpty@ObjectGuid@@')
lookup, = sites(body, '?GetPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVWorldObject@@VObjectGuid@@@Z')
clear, = sites(body, '?Clear@ObjectGuid@@')
valid, = sites(body, '?CanArrive@SilverHandProcession@@')
credit, = sites(body, '?UpdateAchievementCriteria@Player@@')
assert empty < lookup < clear < valid < credit
assert body[empty - 1].op_str == 'rcx, [rdi + 0xe0]'
assert body[empty + 1].op_str == 'al, al' and body[empty + 2].mnemonic == 'jne'
assert not reaches(body, body[empty + 2].operands[0].imm, body[credit].address)
assert any(entry.op_str == 'xmm0, xmmword ptr [rdi + 0xe0]' for entry in body[empty:lookup])
assert any(entry.op_str == 'rcx, qword ptr [rdi + 0x20]' for entry in body[empty:lookup])
assert body[clear - 2].op_str == 'rcx, [rdi + 0xe0]' and body[clear - 1].op_str == 'rsi, rax'
assert body[valid - 1].op_str == 'rcx, rsi'
assert body[valid + 1].op_str == 'al, al' and body[valid + 2].mnemonic == 'je'
assert not reaches(body, body[valid + 2].operands[0].imm, body[credit].address)
for operands in ('ebx, 2', 'esi, 0xe'):
    position = next(index for index, entry in enumerate(body[:empty]) if entry.mnemonic == 'cmp' and entry.op_str == operands)
    assert body[position + 1].mnemonic == 'jne'
    assert not reaches(body, body[position + 1].operands[0].imm, body[empty].address)
talks = [position for position in sites(body, '?Talk@CreatureAI@@QEAAXE') if valid < position < credit]
assert len(talks) == 1 and any(entry.op_str == 'dl, 5' for entry in body[valid:talks[0]])
assert all(any(entry.op_str == operands for entry in body[credit - 8:credit]) for operands in
           ('edx, 0x5c', 'r8d, 0xc6d1', 'rcx, rsi'))
assert not sites(body, '?FindNearestPlayer@WorldObject@@')
for method in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?SetFlag@Object@@', '?SetUInt32Value@Object@@'):
    assert not sites(body, method)
table, = previous['tables']
assert struct.unpack_from('<Q', native['binary'], native['offset'](table) + 0x1a8)[0] == native['symbol'](movement)
factory = instructions('?GetAI@npc_travar_second@@')
constructor, = sites(factory, '??0ObjectGuid@@QEAA@XZ')
assert factory[constructor - 1].op_str == 'rcx, [rsi + 0xe0]'
gossip = instructions('?sGossipSelect@npc_travar_secondAI@npc_travar_second@@')
guid, = sites(gossip, '?GetGuidValue@Object@@')
move, = sites(gossip, '?MovePath@MotionMaster@@')
assert guid < move and gossip[guid - 1].op_str == 'rcx, r14'
assert any(entry.op_str == 'edx, edx' and entry.mnemonic == 'xor' for entry in gossip[guid - 3:guid])
assert gossip[guid + 1].op_str == 'xmm0, xmmword ptr [rax]'
assert gossip[guid + 2].op_str == 'xmmword ptr [rdi + 0xe0], xmm0'
waypoint = instructions('?OnArrived@?$WaypointMovementGenerator@VCreature@@')
dispatch = next(index for index, entry in enumerate(waypoint) if entry.mnemonic == 'mov' and entry.op_str == 'r9, qword ptr [r8 + 0x1a8]')
assert waypoint[dispatch - 1].op_str == 'r8, qword ptr [rcx]'
assert waypoint[dispatch + 1].op_str == 'r8d, dword ptr [r15]'
assert waypoint[dispatch + 2].op_str == 'edx, 2'
assert waypoint[dispatch + 3].mnemonic == 'call' and waypoint[dispatch + 3].op_str == 'r9'
assert any(entry.op_str == 'r15, [rsi + 0x18]' for entry in waypoint[:dispatch])
update = instructions('?DoUpdate@?$WaypointMovementGenerator@VCreature@@')
arrival, = sites(update, '?OnArrived@?$WaypointMovementGenerator@VCreature@@')
assert update[arrival - 4].op_str == 'byte ptr [rax + 0x68], 0x20'
assert update[arrival - 3].mnemonic == 'je'
assert not reaches(update, update[arrival - 3].operands[0].imm, update[arrival].address)
print('PASS: actual second factory/native waypoint dispatcher selects MovementInform slot0x1a8; WAYPOINT2/node14 alone consumes native empty-initialized full128-bit AI+0xe0, resolves original map/world/predelete Player, clears before live Holy1092/exact4/100yd admission and Talk5/92+50897. Raw checked5yd gossip stores real GUID0 before10993806, never credits on request or substitutes a neighbor. Interrupted-motion recovery, party/faction/native counters and client traversal remain separate.')
