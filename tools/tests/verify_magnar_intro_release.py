"""Inspect the deployed native reveal launch, acknowledgement and idle retry."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_guards_release.py')))
native, instructions, positions = (previous[name] for name in ('native', 'instructions', 'positions'))
suffix = '@npc_hrutnirAI@npc_hrutnir@@'
update_name = '?UpdateMagnarIntro' + suffix
cancel_name = '?CancelMagnarIntro' + suffix
player_name = '?IsMagnarIntroPlayer' + suffix
context_name = '?IsMagnarOfferContext' + suffix


def find(body, operand, mnemonic=None):
    return next(index for index, entry in enumerate(body)
                if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))


tables = previous['previous']['previous']['previous']['selected_tables']
assert len(tables('?GetAI@npc_hrutnir@@', ((0x48, '?DoAction' + suffix), (0x18, '?UpdateAI' + suffix)))) == 1
action = instructions('?DoAction' + suffix)
admit = positions(action, context_name)[0]
claim = find(action, 'byte ptr [rbx + 0x88], 1', 'mov')
for operand in ('byte ptr [rcx + 0x88], 0', 'byte ptr [rcx + 0x89], 0'):
    gate = find(action, operand, 'cmp')
    assert gate < admit < claim and action[gate + 1].mnemonic == 'jne'
assert claim < positions(action, '?StopAttack@Creature@@')[0]
assert not positions(action, '?MovePoint@MotionMaster@@'), 'Guard action still walks before the scene'
body = instructions('?UpdateAI' + suffix)
assert positions(body, update_name)[0] < positions(body, '?UpdateVictim@CreatureAI@@')[0]

body = instructions(update_name)
for operand, branch in (('byte ptr [rcx + 0x88], 0', 'je'), ('byte ptr [rcx + 0xa4], 0', 'jne')):
    gate = find(body, operand, 'cmp')
    assert body[gate + 1].mnemonic == branch
context = positions(body, context_name)
assert len(context) == 2
clock = find(body, 'eax, ebx', 'cmp')
assert context[0] < clock and body[clock + 1].mnemonic == 'jbe'
assert find(body, 'dword ptr [rsi + 0x9c], 0x1f4', 'mov') > clock
statuses = positions(body, '?HasSceneStatus@Player@@')
talk = positions(body, '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')[0]
move = positions(body, '?MovePoint@MotionMaster@@')[0]
claim = find(body, 'word ptr [rsi + 0x88], 0x100', 'mov')
assert len(statuses) == 2 and statuses[0] < claim < talk < move < statuses[1]
assert any(entry.op_str == 'dword ptr [rsp + 0x40], 3' for entry in body[:statuses[0]])
assert body[statuses[0] - 2].op_str == 'edx, 0x423' and body[statuses[0] + 2].mnemonic == 'je'
assert body[talk - 2].op_str == 'dl, 0xe' and body[move - 1].op_str == 'edx, 6'
coordinates = []
for entry in body[talk:move]:
    if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        coordinates.append(struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))[0])
assert len(coordinates) == 3 and sorted(round(value, 2) for value in coordinates) == [-37.19, 340.87, 4763.73]
lookup = positions(body, '?GetSpellScene@SpellMgr@@')[0]
cast = positions(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@@Z')[0]
for operand in ('dword ptr [rax], 0x5b2', 'dword ptr [rax + 8], 0x10',
                'dword ptr [rax + 0xc], 0', 'dword ptr [rax + 0x10], 0'):
    gate = find(body, operand, 'cmp')
    assert lookup < gate < cast and body[gate + 1].mnemonic == 'jne'
events = positions(body, '?GetSceneTriggerEvent@SpellMgr@@')[0]
assert lookup < events < cast and body[events + 2].mnemonic == 'jne'
assert body[lookup - 2].op_str == body[events - 2].op_str == 'edx, 0x423'
assert find(body, 'xmmword ptr [rsi + 0x8b], xmm0', 'movups') < find(body, 'byte ptr [rsi + 0xa4], 1', 'mov') < cast
assert [entry.op_str for entry in body[cast - 4:cast]] == ['r9b, 1', 'r8d, 0x2e519', 'rdx, r14', 'rcx, r14']
epoch = find(body, 'ebx, dword ptr [rsi + 0xa0]', 'cmp')
assert cast < epoch < context[1] < statuses[1] and body[epoch + 1].mnemonic == 'je'
assert find(body, 'edi, edi', 'test') > context[1]
auras = positions(body, '?HasAura@Unit@@QEBA_NIVObjectGuid@@0I@Z')
assert len(auras) == 3 and cast < auras[2] < statuses[1]
assert any(entry.op_str == 'dword ptr [rsp + 0x40], 1' for entry in body[auras[2]:statuses[1]])
assert statuses[1] < positions(body, cancel_name)[-1]
assert not positions(body, '?UpdateAchievementCriteria@Player@@') and not positions(body, '?KilledMonsterCredit@Player@@')

body = instructions(player_name)
deny = body[find(body, 'rdx, rdx', 'test') + 1].operands[0].imm
for operand, branch in (('dword ptr [rbx + 0xa18], 0', 'jne'), ('byte ptr [rbx + 0x4ff0], 1', 'jne'),
                        ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                        ('al, 1', 'jne'), ('qword ptr [rbx + 8], rax', 'jne'), ('dword ptr [rbx + 0x3b8], eax', 'jne')):
    gate = find(body, operand)
    assert body[gate + 1].mnemonic == branch and body[gate + 1].operands[0].imm == deny
for dependency in ('?IsInWorld@Object@@', '?CanContact@Player@@', '?InSamePhase@WorldObject@@', '?IsWithinDistInMap@WorldObject@@'):
    gate = positions(body, dependency)[0]
    assert body[gate + 2].mnemonic == 'je' and body[gate + 2].operands[0].imm == deny
distance = next(entry for entry in body if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM)
assert struct.unpack_from('<f', native['binary'], native['offset'](distance.address + distance.size + distance.operands[1].mem.disp))[0] == 150.0
reward = positions(body, '?GetQuestRewardStatus@Player@@')[0]
assert body[reward - 2].op_str == 'edx, 0x9917' and body[reward + 2].mnemonic == 'jne'
assert len(positions(body, '?GetQuestStatus@Player@@')) == 2
assert find(body, 'eax, 3', 'cmp') < find(body, 'eax, 1', 'cmp')

body = instructions(cancel_name)
clear = positions(body, '?Clear@ObjectGuid@@')[0]
lookup = positions(body, '?GetPlayer@ObjectAccessor@@')[0]
remove = positions(body, '?RemoveAurasDueToSpell@Unit@@')[0]
assert clear < find(body, 'dword ptr [rbx + 0xa0]', 'inc') < lookup < remove
assert body[remove - 3].op_str == 'edx, 0x2e519'
for name in ('?Reset' + suffix, '??_Gnpc_hrutnirAI@npc_hrutnir@@'):
    assert len(positions(instructions(name), cancel_name)) == 1
print('PASS: canonical Hruthnir action2/idle UpdateAI vtable slots select native189721/1059/package1458; scoped39191 owner, cast/aura/launch/epoch guards and500ms retry precede actual completion-only Talk14/point6. Reset/destructor cancel owned local aura; no new credit/item grants. Client camera/VFX, physical arrival and saved re-entry remain separate.')
