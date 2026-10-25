"""Verify second-Travard stage admission and native packet-selected procession."""
from pathlib import Path
import math
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_arrival_release.py')))
native, instructions = previous['native'], previous['instructions']
sites, reaches = previous['sites'], previous['reaches']
helper = '?CanStart@SilverHandProcession@@'
body = instructions(helper)
deny = next(entry.address for index, entry in enumerate(body[:-1]) if entry.op_str == 'rbx, qword ptr [rsp + 0x40]'
            and body[index + 1].op_str == 'al, al' and body[index + 1].mnemonic == 'xor')


def reject(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == deny


for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('dword ptr [rdi + 0xa18], 0', 'jne'), ('dword ptr [rdi + 0xe0], 0x603', 'jne'),
                         ('rax, rax', 'je'), ('rax, qword ptr [rdi + 8]', 'jne'),
                         ('dword ptr [rbx + 0x3b8], eax', 'jne')):
    position = next(index for index, entry in enumerate(body) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test'))
    reject(position, branch)
for method in ('?IsInWorld@Object@@', '?CanContact@Player@@', '?InSamePhase@WorldObject@@',
               '?canSeeOrDetect@WorldObject@@', '?IsWithinLOSInMap@WorldObject@@', '?IsWithinDistInMap@WorldObject@@'):
    positions = sites(body, method)
    assert len(positions) == (2 if method == '?IsInWorld@Object@@' else 1)
    for position in positions:
        assert body[position + 1].op_str == 'al, al'
        reject(position + 1, 'je')
entry, = sites(body, '?GetUInt32Value@Object@@')
assert body[entry + 1].op_str == 'eax, 0x1a08f'
reject(entry + 1, 'jne')
klass, = sites(body, '?GetByteValue@Object@@')
assert body[klass + 1].op_str == 'al, 2'
reject(klass + 1, 'jne')
rewarded, = sites(body, '?GetQuestRewardStatus@Player@@')
quest, = sites(body, '?GetQuestStatus@Player@@')
for position in (rewarded, quest):
    assert any(entry.op_str == 'edx, 0xa488' for entry in body[position - 3:position])
assert body[rewarded + 1].op_str == 'al, al'
reject(rewarded + 1, 'jne')
assert body[quest + 1].op_str == 'eax, 3'
reject(quest + 1, 'jne')
scenario, = sites(body, '?GetScenario@ScenarioMgr@@')
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
identity, = sites(body, '?GetScenarioId@Scenario@@')
assert body[identity + 1].op_str == 'eax, 0x444'
reject(identity + 1, 'jne')
step, = sites(body, '?GetCurrentStep@Scenario@@')
assert body[step + 1].op_str == 'eax, 4'
reject(step + 1, 'jne')


def floats(decoded):
    return [struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))[0]
            for entry in decoded if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM
            and entry.operands[1].mem.base == X86_REG_RIP]


distance, = sites(body, '?IsWithinDistInMap@WorldObject@@')
assert floats(body[distance - 7:distance]) == [5.0]
prefix = '?sGossipSelect@npc_travar_secondAI@npc_travar_second@@'
body = instructions(prefix)
consume, = sites(body, '?RemoveFlag@Object@@')
clear, = sites(body, '?ClearMenus@PlayerMenu@@')
close, = sites(body, '?SendCloseGossip@PlayerMenu@@')
move, = sites(body, '?MovePath@MotionMaster@@')
talk, = sites(body, '?Talk@CreatureAI@@QEAAXE')
assert consume < clear < close < move < talk
assert body[0].op_str == 'r8d, 0x4c8f' and body[1].mnemonic == 'jne'
assert not reaches(body, body[1].operands[0].imm, body[consume].address)
for operands, branch in (('r9d, r9d', 'jne'), ('eax, 0x4c8f', 'jne'), ('rsi, rsi', 'je'),
                         ('byte ptr [rsi], 0', 'jne'), ('dword ptr [rsi + 4], 0', 'jne'),
                         ('dword ptr [rsi + 8], 0', 'jne'), ('byte ptr [rsi + 0x50], 0', 'jne')):
    position = next(index for index, entry in enumerate(body[:consume]) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test'))
    assert body[position + 1].mnemonic == branch
    assert not reaches(body, body[position + 1].operands[0].imm, body[consume].address)
for method in (helper, '?HasFlag@Object@@'):
    position, = sites(body, method)
    assert body[position + 1].op_str == 'al, al' and body[position + 2].mnemonic == 'je'
    assert not reaches(body, body[position + 2].operands[0].imm, body[consume].address)
flag, = sites(body, '?HasFlag@Object@@')
for position in (flag, consume):
    assert all(any(entry.op_str == operands for entry in body[position - 5:position]) for operands in ('edx, 0x7d', 'r8d, 1'))
item, = sites(body, '?GetItem@GossipMenu@@')
assert any(entry.mnemonic == 'xor' and entry.op_str == 'edx, edx' for entry in body[item - 4:item])
assert body[item + 1].op_str == 'rsi, rax'
previous['previous']['verify_script_path'](body, 10993806, consume)
assert any(entry.op_str == 'edx, 0xa7c08e' for entry in body[move - 5:move])
assert any(entry.mnemonic == 'xor' and entry.op_str == 'r8d, r8d' for entry in body[move - 5:move])
assert any(entry.mnemonic == 'xor' and entry.op_str == 'edx, edx' for entry in body[talk - 5:talk])
followers = sites(body, '?FindNearestCreature@WorldObject@@')
follows = sites(body, '?MoveFollow@MotionMaster@@')
assert len(followers) == len(follows) == 2
assert close < followers[0] < follows[0] < followers[1] < follows[1] < move
for position, follow, identity in zip(followers, follows, (106371, 106370)):
    assert any(entry.op_str == f'edx, {identity:#x}' for entry in body[position - 5:position])
    assert any(entry.op_str == 'r9b, 1' for entry in body[position - 5:position])
    assert floats(body[position - 5:position]) == [30.0]
    assert any(entry.op_str == 'dword ptr [rsp + 0x20], 0' for entry in body[follow - 6:follow])
    assert body[position + (2 if position == followers[0] else 1)].op_str == 'rax, rax'
angles = [floats(body[follow - 6:follow])[0] for follow in follows]
assert math.isclose(angles[0], -math.pi / 2, abs_tol=0.00001) and math.isclose(angles[1], math.pi / 2, abs_tol=0.00001)
assert not any(name.startswith('?OnGossipSelect@npc_travar_second@@') for name in native['symbols'])
for method in ('?SetUInt32Value@Object@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@', '?AddItem@Player@@'):
    assert not sites(body, method)
tables = []
for entry in instructions('?GetAI@npc_travar_second@@'):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0xc8)[0] == native['symbol'](prefix):
            tables.append(address)
assert len(tables) == 1, 'Factory does not select guarded second-Travard native AI gossip slot0xc8'
print('PASS: actual second-Travard factory selects native packet AI slot0xc8; raw19599/0, stored None0 menu item, live Holy42120/1092/exact4/5yd and GOSSIP gate before real script-path10993806. Missing/empty path retains readiness; only GOSSIP removed/own menus cleared before original alive30yd Efrin/Duval followers, one path and Talk0. No direct progression/item or physical movement/client proof.')
