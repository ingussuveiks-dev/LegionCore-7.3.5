"""Verify selected ritual POINT1, shared eligible reference and deferred completion."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_ritual_action_release.py')))
native, instructions, sites = previous['native'], previous['instructions'], previous['sites']
movement_name = '?MovementInform@npc_travar_thirdAI@'
complete_name = '?CompleteRitualArrival@npc_travar_thirdAI@'
movement, complete = instructions(movement_name), instructions(complete_name)
factory = instructions('?GetAI@npc_travar_third@@')
tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x1a8)[0] == native['symbol'](movement_name):
            tables.append(address)
assert len(tables) == 1
slots = struct.unpack_from('<80Q', native['binary'], native['offset'](tables[0]))
assert slots[0x110 // 8] == native['symbol']('?UpdateLifecycle@npc_travar_thirdAI@')
assert any(entry.op_str == 'dword ptr [rsi + 0x168], ebp' for entry in factory)
assert any(entry.op_str == 'byte ptr [rsi + 0x16c], bpl' for entry in factory)
point = next(index for index, entry in enumerate(movement) if entry.op_str == 'esi, 8' and entry.mnemonic == 'cmp')
finish, = sites(movement, complete_name)
exit_address = movement[point + 1].operands[0].imm
for operands, branch in (('esi, 8', 'jne'), ('edi, 1', 'jne'), ('byte ptr [rbx + 0x16a], 0', 'je'), ('byte ptr [rbx + 0x16b], 0', 'jne')):
    position = next(index for index, entry in enumerate(movement[point:finish], point) if entry.op_str == operands)
    assert movement[position + 1].mnemonic == branch and movement[position + 1].operands[0].imm == exit_address
assert movement[finish - 2].op_str == 'byte ptr [rbx + 0x16b], dil'
for method in ('?FindNearestPlayer@WorldObject@@', '?UpdateAchievementCriteria@Player@@', '?DespawnOrUnsummon@Creature@@'):
    assert not sites(movement[point:], method)
deny = complete[-3].address
assert complete[-3].mnemonic == 'add' and complete[-1].mnemonic == 'ret'
for operands, branch in (('byte ptr [rcx + 0x16a], 0', 'je'), ('byte ptr [rcx + 0x16b], 0', 'je'), ('byte ptr [rcx + 0x16c], 0', 'jne')):
    position = next(index for index, entry in enumerate(complete) if entry.op_str == operands)
    following = next(entry for entry in complete[position + 1:] if entry.mnemonic.startswith('j'))
    assert following.mnemonic == branch and following.operands[0].imm == deny
advance = sites(complete, '?CanAdvance@SilverHandRitualEntry@@')
assert len(advance) == 2
assert complete[advance[0] + 2].mnemonic == 'je' and complete[advance[0] + 2].operands[0].imm > complete[advance[1]].address
assert any(entry.op_str == 'rcx, qword ptr [rax + 0x768]' for entry in complete[advance[0]:advance[1]])
reference = next(index for index, entry in enumerate(complete) if entry.op_str == 'rdi, qword ptr [rbx + 0x20]')
skip = next(entry.address for entry in complete if entry.op_str == 'rbx, qword ptr [rbx + 8]')
assert complete[advance[1] + 2].mnemonic == 'je' and complete[advance[1] + 2].operands[0].imm == skip
assert complete[reference + 2].mnemonic == 'je' and complete[reference + 2].operands[0].imm == skip
for method, branch in (('?IsInWorld@Object@@', 'je'), ('?IsPreDelete@Object@@', 'jne'), ('?CanContact@Player@@', 'je'),
                       ('?InSamePhase@WorldObject@@', 'je'), ('?IsWithinDistInMap@WorldObject@@', 'je'), ('?GetQuestRewardStatus@Player@@', 'jne')):
    position, = sites(complete, method)
    assert complete[position + 1].op_str == 'al, al'
    assert complete[position + 2].mnemonic == branch and complete[position + 2].operands[0].imm == skip
for operands in ('byte ptr [rdi + 0x4ff0], 1', 'byte ptr [rdi + 0x6848], r14b', 'byte ptr [rdi + 0x6849], r14b',
                 'al, 2', 'qword ptr [rdi + 8], rax', 'dword ptr [rdi + 0x3b8], eax'):
    position = next(index for index, entry in enumerate(complete) if entry.op_str == operands)
    assert complete[position + 1].mnemonic == 'jne' and complete[position + 1].operands[0].imm == skip
assert any(entry.op_str == 'r14d, r14d' and entry.mnemonic == 'xor' for entry in complete[:reference])
radius, = [entry for entry in complete[:reference] if entry.mnemonic == 'movss' and entry.op_str.startswith('xmm6, dword ptr [rip')]
assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 100.0
for method in ('?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@'):
    position, = sites(complete, method)
    assert complete[position - 2].op_str == 'edx, 0xa488'
status, = sites(complete, '?GetQuestStatus@Player@@')
assert complete[status + 1].op_str == 'eax, 3' and complete[status + 2].mnemonic == 'je'
accepted = complete[status + 2].operands[0].imm
backedge = [entry for entry in complete[status + 3:] if entry.mnemonic == 'jne' and entry.operands[0].imm == complete[reference].address]
assert len(backedge) == 1 and accepted > backedge[0].address
despawn, = sites(complete, '?DespawnOrUnsummon@Creature@@')
credit, = sites(complete, '?UpdateAchievementCriteria@Player@@')
assert any(entry.op_str == 'byte ptr [rbp + 0x16c], 1' for entry in complete[status:despawn])
assert any(entry.op_str == 'edx, 0x3e8' for entry in complete[status:despawn])
assert any(entry.op_str == 'edx, 0x5c' for entry in complete[despawn:credit])
assert any(entry.op_str == 'r8d, 0xc6e3' for entry in complete[despawn:credit])
assert complete[credit - 1].op_str == 'rcx, rbx'
assert not sites(complete, '?FindNearestPlayer@WorldObject@@') and not sites(complete, '?AddItem@Player@@')
lifecycle = instructions('?UpdateLifecycle@npc_travar_thirdAI@')
recover, = sites(lifecycle, '?Recover@SilverHandCompanion@@')
retry, = sites(lifecycle, complete_name)
assert recover < retry and lifecycle[retry - 1].op_str == 'rcx, rbx'
delayed = instructions(previous['closure'])
talk, = sites(delayed, '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')
move, = sites(delayed, '?MovePoint@MotionMaster@@')
assert any(entry.op_str == 'byte ptr [rax + 0x16a], 1' for entry in delayed[talk:move])
print('PASS: selected MovementInform0x1a8 admits issued POINT8/id1 once, latches actual arrival before completion; selected Lifecycle0x110 retries without motion. Inlined actual-map references skip invalid/GM/teleport/nonPaladin/foreign-instance/phase/distant/inactive42120 participants, retain100yd/exact1092-step8 and choose one shared Player. Completion guards issued/arrived/completed, latches before despawn1000 and one92/50915. Factory initializes all flags; no nearest substitute/manual group broadcast/item grant. Not persisted quest-attempt or client traversal proof.')
