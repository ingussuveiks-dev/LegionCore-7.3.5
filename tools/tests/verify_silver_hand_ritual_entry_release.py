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
           ('qword ptr [rsp + 0x48], xmm0', 'qword ptr [rsp + 0x50], rax', 'rax, qword ptr [rdi + 0x160]',
            'qword ptr [rsp + 0x58], rax', 'qword ptr [rsp + 0x60], rdi', 'rcx, [rdi + 0xe0]'))
assert any(entry.op_str == 'byte ptr [rdi + 0x168], 1' for entry in body[gate:guid])
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
assert any(entry.op_str == 'xmm0, xmmword ptr [rbx]' for entry in delayed[:lookup])
assert any(entry.op_str == 'rcx, qword ptr [rbx + 0x18]' for entry in delayed[:lookup])
pending = next(index for index, entry in enumerate(delayed) if entry.op_str == 'byte ptr [rcx + 0x168], 0' and entry.mnemonic == 'cmp')
generation = next(index for index, entry in enumerate(delayed) if entry.op_str == 'qword ptr [rbx + 0x10], rax' and entry.mnemonic == 'cmp')
assert pending < generation < lookup
assert any(entry.op_str == 'rax, qword ptr [rcx + 0x160]' for entry in delayed[pending:generation])
for position, branch in ((pending, 'je'), (generation, 'jne')):
    assert delayed[position + 1].mnemonic == branch and not reaches(delayed, delayed[position + 1].operands[0].imm, delayed[talk].address)
assert any(entry.op_str == 'byte ptr [rcx + 0x168], 0' and entry.mnemonic == 'mov' for entry in delayed[generation:lookup])
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
slots = struct.unpack_from('<80Q', native['binary'], native['offset'](selected[0]))
reset_name = '?Reset@npc_travar_thirdAI@'
lifecycle_name = '?UpdateLifecycle@npc_travar_thirdAI@'
destructor_name = '??_Gnpc_travar_thirdAI@'
assert slots[0x28 // 8] == native['symbol'](reset_name) and slots[0x110 // 8] == native['symbol'](lifecycle_name)
assert slots[0] == native['symbol'](destructor_name)
factory = instructions('?GetAI@npc_travar_third@@')
constructors = sites(factory, '??0FunctionProcessor@@')
assert len(constructors) == 2
assert [factory[position - 1].op_str for position in constructors] == ['rcx, [rsi + 0x60]', 'rcx, [rsi + 0xe0]']
assert any(entry.op_str == 'qword ptr [rsi + 0x160], rbp' for entry in factory)
assert any(entry.op_str == 'byte ptr [rsi + 0x168], bpl' for entry in factory)
assert any(entry.mnemonic == 'xor' and entry.op_str == 'ebp, ebp' for entry in factory)
reset_body = instructions(reset_name)
guard = next(index for index, entry in enumerate(reset_body) if entry.op_str == 'byte ptr [rbx + 0x168], 0' and entry.mnemonic == 'cmp')
assert reset_body[guard + 2].mnemonic == 'je'
assert [entry.op_str for entry in reset_body[guard + 3:guard + 6]] == [
    'qword ptr [rbx + 0x160]', 'byte ptr [rbx + 0x168], 0', 'byte ptr [rbx + 0x59], 0']
assert reset_body[guard + 3].mnemonic == 'inc'
assert not reaches(reset_body, reset_body[guard + 2].operands[0].imm, reset_body[guard + 3].address)
assert not sites(reset_body, '?KillAllFunctions@FunctionProcessor@@')
lifecycle_body = instructions(lifecycle_name)
recover, = sites(lifecycle_body, '?Recover@SilverHandCompanion@@')
assert any(entry.op_str == 'rcx, [rbx + 0xe0]' for entry in lifecycle_body[recover + 1:])
assert lifecycle_body[-1].mnemonic == 'jmp' and native['resolve'](lifecycle_body[-1].operands[0].imm) == native['symbol']('?Update@FunctionProcessor@@')
destroy_body = instructions(destructor_name)
destructors = sites(destroy_body, '??1FunctionProcessor@@')
assert len(destructors) == 2 and destroy_body[destructors[0] - 1].op_str == 'rcx, 0xe0'
assert destroy_body[destructors[1] - 1].op_str == 'rcx, [rdi + 0x60]'
creature = instructions('?Update@Creature@@')
ticks = [index for index, entry in enumerate(creature) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x110]
assert len(ticks) == 2
assert any(entry.op_str == 'byte ptr [rsi + 0x3a9d], 1' for entry in creature[ticks[0] - 10:ticks[0]])
assert creature[ticks[1] + 1].op_str == 'byte ptr [rsi + 0x3a9d], 0'
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c8]' for entry in creature[ticks[0] - 8:ticks[0]])
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c0]' for entry in creature[ticks[0]:ticks[1]])
assert min(sites(creature, '?Update@Unit@@')) > ticks[1]
print('PASS: selected third-Travard LOS0x270 owns its1s queue atAI+0xe0, captures full128-bit original GUID/64-bit generation and arms pending state. Selected locked lifecycle0x110 services it independently of stunned/combat AI; selected destructor destroys both queues. Pending-only Reset increments generation/reopens admission without replaying an accepted route or deferred KillAllFunctions eating a fresh retry. Actual closure rejects stale generation, consumes pending before native original lookup/live1092 steps5..8/20yd/path10993807 preflight and original Talk0/30yd followers. No Unit-owned AI callback, Player pointer, nearby substitute or direct credit. Other script timers/downstream rituals/client traversal remain separate.')
