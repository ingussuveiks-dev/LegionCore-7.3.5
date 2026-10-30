"""Verify the real inlined POINT1 recipient bridge and its full-width pending GUID."""
from pathlib import Path
import runpy
import struct

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_gossip_release.py')))
native, instructions = previous['native'], previous['instructions']
sites, reaches = previous['sites'], previous['reaches']
movement = '?MovementInform@npc_travar_firstAI@npc_travar_first@@'
body = instructions(movement)
lookup, = sites(body, '?GetPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVWorldObject@@VObjectGuid@@@Z')
empty, = sites(body, '?IsEmpty@ObjectGuid@@')
clear, = sites(body, '?Clear@ObjectGuid@@')
valid, = sites(body, '?CanArrive@SilverHandEntrance@@')
flag, = sites(body, '?SetFlag@Object@@')
criteria = [position for position in sites(body, '?UpdateAchievementCriteria@Player@@') if position > lookup]
assert len(criteria) == 1
credit = criteria[0]
assert empty < lookup < clear < valid < flag < credit
assert body[empty - 1].op_str == 'rcx, [rdi + 0xe0]'
assert body[empty + 1].op_str == 'al, al' and body[empty + 2].mnemonic == 'jne'
assert not reaches(body, body[empty + 2].operands[0].imm, body[flag].address)
assert any(entry.op_str == 'xmm0, xmmword ptr [rdi + 0xe0]' for entry in body[empty:lookup])
assert body[clear - 1].op_str == 'rcx, [rdi + 0xe0]'
assert body[valid - 1].op_str == 'rcx, r14' and body[lookup + 1].op_str == 'r14, rax'
assert body[valid + 1].op_str == 'al, al' and body[valid + 2].mnemonic == 'jne'
assert body[valid + 3].op_str == 'byte ptr [rdi + 0x58], al'
assert not reaches(body, body[valid + 3].address, body[flag].address)
assert any(entry.op_str == 'dl, 2' for entry in body[valid:flag])
assert any(entry.op_str == 'edx, 0x7d' for entry in body[flag - 5:flag])
assert any(entry.op_str == 'r8d, 1' for entry in body[flag - 5:flag])
assert any(entry.op_str == 'edx, 0x5c' for entry in body[credit - 8:credit])
assert any(entry.op_str == 'r8d, 0xc64d' for entry in body[credit - 8:credit])
assert any(entry.op_str == 'rcx, r14' for entry in body[credit - 8:credit])
assert body[credit + 1].op_str == 'byte ptr [rdi + 0x5a], 1'
arrived = body[empty - 1].address
point = next(index for index, entry in enumerate(body) if entry.op_str == 'ebx, 8' and entry.mnemonic == 'cmp')
assert body[point + 1].mnemonic == 'jne' and not reaches(body, body[point + 1].operands[0].imm, arrived)
dispatch = [index for index, entry in enumerate(body[:empty]) if entry.mnemonic == 'je' and entry.operands[0].imm == arrived]
assert len(dispatch) == 1 and body[dispatch[0] - 1].op_str == 'r14d, 1'
assert body[dispatch[0] - 1].mnemonic == 'sub'
region = body[empty:]
assert not sites(region, '?FindNearestPlayer@WorldObject@@')
assert len(sites(region, '?MoveFollow@MotionMaster@@')) == 2
for identity in ('edx, 0x19f83', 'edx, 0x19f82'):
    assert any(entry.op_str == identity for entry in region)
factory = instructions('?GetAI@npc_travar_first@@')
constructors = sites(factory, '??0ObjectGuid@@QEAA@XZ')
constructor, = [i for i in constructors if factory[i - 1].op_str == 'rcx, [rsi + 0xe0]']
assert len(constructors) == 2
boss, = [i for i in constructors if i != constructor]
assert [entry.op_str for entry in factory[boss - 2:boss]] == ['rbx, [rsi + 0x180]', 'rcx, rbx']
assert factory[constructor - 1].op_str == 'rcx, [rsi + 0xe0]'
delayed = previous['previous']['delayed']
move, = sites(delayed, '?MovePoint@MotionMaster@@')
store = next(index for index, entry in enumerate(delayed[:move]) if entry.op_str == 'xmmword ptr [rax + 0xe0], xmm0')
assert any(entry.op_str == 'xmm0, xmmword ptr [rbx + 0x10]' for entry in delayed[store - 6:store])
table, = previous['previous']['selected']
assert struct.unpack_from('<Q', native['binary'], native['offset'](table) + 0x1a8)[0] == native['symbol'](movement)
print('PASS: selected POINT8/id1 MovementInform contains actual inlined arrival bridge; native empty-initialized full128-bit AI+0xe0 captures original delayed recipient before MovePoint, resolves and clears before live revalidation/50765/readiness. Empty/invalid/duplicate/reentrant arrival cannot publish gossip or substitute nearby player; only gossip bit, original follower roles and native92/50765 call retained. Later waypoint/group/native-counter/client traversal remain separate.')
