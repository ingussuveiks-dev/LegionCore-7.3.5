"""Verify selected third-Travard LOS and its GUID-bound delayed entry callback."""
from pathlib import Path
import math
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_procession_release.py')))
native, instructions = previous['native'], previous['instructions']
sites, reaches = previous['sites'], previous['reaches']
helper = '?CanEnter@SilverHandRitualEntry@@'
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
for method, operands in (('?GetUInt32Value@Object@@', 'eax, 0x1a0b4'), ('?GetByteValue@Object@@', 'al, 2'),
                         ('?GetQuestStatus@Player@@', 'eax, 3'), ('?GetScenarioId@Scenario@@', 'eax, 0x444')):
    position, = sites(body, method)
    assert body[position + 1].op_str == operands
    reject(position + 1, 'jne')
rewarded, = sites(body, '?GetQuestRewardStatus@Player@@')
assert body[rewarded + 1].op_str == 'al, al'
reject(rewarded + 1, 'jne')
quest, = sites(body, '?GetQuestStatus@Player@@')
for position in (rewarded, quest):
    assert any(entry.op_str == 'edx, 0xa488' for entry in body[position - 3:position])
scenario, = sites(body, '?GetScenario@ScenarioMgr@@')
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
step, = sites(body, '?GetCurrentStep@Scenario@@')
assert body[step + 1].op_str == 'eax, -5' and body[step + 1].mnemonic == 'add'
assert body[step + 2].op_str == 'eax, 3' and body[step + 3].mnemonic == 'setbe'


def floats(decoded):
    return [struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))[0]
            for entry in decoded if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM
            and entry.operands[1].mem.base == X86_REG_RIP]


distance, = sites(body, '?IsWithinDistInMap@WorldObject@@')
assert floats(body[distance - 7:distance]) == [20.0]
los = '?MoveInLineOfSight@npc_travar_thirdAI@npc_travar_third@@'
body = instructions(los)
gate, = sites(body, helper)
arm, = sites(body, '?AddDelayedEvent@FunctionProcessor@@')
guid, = sites(body, '?GetGuidValue@Object@@')
assert gate < guid < arm
assert body[gate + 1].op_str == 'al, al' and body[gate + 2].mnemonic == 'je'
assert not reaches(body, body[gate + 2].operands[0].imm, body[arm].address)
latch = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rcx + 0x59], 0')
assert body[latch + 1].mnemonic == 'jne' and not reaches(body, body[latch + 1].operands[0].imm, body[arm].address)
assert any(entry.op_str == 'byte ptr [rdx + 0x16], 4' for entry in body[:gate])
assert any(entry.op_str == 'rdx, rdx' and entry.mnemonic == 'test' for entry in body[:gate])
assert any(entry.op_str == 'edx, 0x3e8' for entry in body[arm - 3:arm])
assert any(entry.op_str == 'edx, edx' and entry.mnemonic == 'xor' for entry in body[guid - 3:guid])
assert any(entry.op_str == 'xmm0, xmmword ptr [rax]' for entry in body[guid:arm])
assert all(any(entry.op_str == operands for entry in body[guid:arm]) for operands in
           ('qword ptr [rsp + 0x48], xmm0', 'qword ptr [rsp + 0x50], rax', 'qword ptr [rsp + 0x58], rdi'))
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'MoveInLineOfSight@npc_travar_thirdAI@npc_travar_third@@' in name]
operation, = [name for name in native['symbols'] if name.startswith('??R<lambda_1>') and 'MoveInLineOfSight@npc_travar_thirdAI@npc_travar_third@@' in name]
installed = []
for entry in body:
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            installed.append(address)
assert len(installed) == 1
address = native['symbol'](closure)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
adapter = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + 9], address))
assert len(adapter) == 2 and adapter[0].op_str == 'rcx, 8' and adapter[0].mnemonic == 'add'
assert adapter[1].mnemonic == 'jmp' and adapter[1].operands[0].imm == native['symbol'](operation)
delayed = instructions(operation)
lookup, = sites(delayed, '?GetPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVWorldObject@@VObjectGuid@@@Z')
valid, = sites(delayed, helper)
talk, = sites(delayed, '?Talk@CreatureAI@@QEAAXE')
move, = sites(delayed, '?MovePath@MotionMaster@@')
assert lookup < valid < talk < move
assert any(entry.op_str == 'xmm0, xmmword ptr [rcx]' for entry in delayed[:lookup])
assert any(entry.op_str == 'rcx, qword ptr [rcx + 0x10]' for entry in delayed[:lookup])
assert delayed[valid - 1].op_str == 'rcx, rax'
assert delayed[valid + 1].op_str == 'al, al' and delayed[valid + 2].mnemonic == 'jne'
assert delayed[valid + 4].op_str == 'byte ptr [rax + 0x59], 0'
assert not reaches(delayed, delayed[valid + 3].address, delayed[talk].address)
ancestor = previous
while 'verify_script_path' not in ancestor:
    ancestor = ancestor['previous']
ancestor['verify_script_path'](delayed, 10993807, talk)
path, = sites(delayed, '?GetPathScript@WaypointMgr@@')
warning = delayed[path + 2].operands[0].imm
reset = next(index for index, entry in enumerate(delayed) if entry.address == warning)
assert delayed[reset + 1].op_str == 'byte ptr [rax + 0x59], 0'
assert any(entry.op_str == 'edx, edx' and entry.mnemonic == 'xor' for entry in delayed[talk - 4:talk])
followers = sites(delayed, '?FindNearestCreature@WorldObject@@')
follows = sites(delayed, '?MoveFollow@MotionMaster@@')
assert len(followers) == len(follows) == 2 and talk < followers[0] < follows[0] < followers[1] < follows[1] < move
for position, follow, identity in zip(followers, follows, (106371, 106370)):
    assert any(entry.op_str == f'edx, {identity:#x}' for entry in delayed[position - 5:position])
    assert floats(delayed[position - 5:position]) == [30.0]
    assert any(entry.op_str == 'r9b, 1' for entry in delayed[position - 5:position])
    assert any(entry.op_str == 'dword ptr [rsp + 0x20], 0' for entry in delayed[follow - 6:follow])
angles = [floats(delayed[position - 6:position])[0] for position in follows]
assert math.isclose(angles[0], -math.pi / 2, abs_tol=0.00001) and math.isclose(angles[1], math.pi / 2, abs_tol=0.00001)
assert any(entry.op_str == 'edx, 0xa7c08f' for entry in delayed[move - 5:move])
for decoded in (body, delayed):
    for method in ('?FindNearestPlayer@WorldObject@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@', '?AddItem@Player@@'):
        assert not sites(decoded, method)
selected = []
for entry in instructions('?GetAI@npc_travar_third@@'):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x270)[0] == native['symbol'](los):
            selected.append(address)
assert len(selected) == 1
print('PASS: selected third-Travard LOS slot0x270 requires live Holy42120/1092/native5..8/20yd and intro admission before actual1s timer. Installed GUID0/full128-bit closure adapter resolves original map/world/predelete Player and revalidates before native10993807, Talk0 and original30yd followers. Invalid recipient or missing/empty path resets only intro latch with retained diagnostics/retry; no Player pointer capture, nearby substitute or direct progression. Later ritual/motion/group/faction/client traversal remains separate.')
