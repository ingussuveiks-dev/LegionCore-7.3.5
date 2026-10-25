"""Verify the selected entrance AI and its GUID-resolved delayed recipient."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_collapse_release.py')))
native, instructions = previous['native'], previous['instructions']
reaches, sites = previous['reaches'], previous['sites']
helper = '?CanArrive@SilverHandEntrance@@'
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
for prefix in ('?IsInWorld@Object@@', '?CanContact@Player@@', '?InSamePhase@WorldObject@@',
               '?canSeeOrDetect@WorldObject@@', '?IsWithinLOSInMap@WorldObject@@', '?IsWithinDistInMap@WorldObject@@'):
    positions = sites(body, prefix)
    assert len(positions) == (2 if prefix == '?IsInWorld@Object@@' else 1)
    for position in positions:
        assert body[position + 1].op_str == 'al, al'
        reject(position + 1, 'je')
entry, = sites(body, '?GetUInt32Value@Object@@')
assert body[entry + 1].op_str == 'eax, 0x19fbd'
reject(entry + 1, 'jne')
klass, = sites(body, '?GetByteValue@Object@@')
assert body[klass + 1].op_str == 'al, 2'
reject(klass + 1, 'jne')
rewarded, = sites(body, '?GetQuestRewardStatus@Player@@')
assert body[rewarded + 1].op_str == 'al, al'
reject(rewarded + 1, 'jne')
quest, = sites(body, '?GetQuestStatus@Player@@')
assert any(entry.op_str == 'edx, 0xa488' for entry in body[quest - 3:quest])
assert body[quest + 1].op_str == 'eax, 3'
reject(quest + 1, 'jne')
scenario, = sites(body, '?GetScenario@ScenarioMgr@@')
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
identity, = sites(body, '?GetScenarioId@Scenario@@')
assert body[identity + 1].op_str == 'eax, 0x444'
reject(identity + 1, 'jne')
step, = sites(body, '?GetCurrentStep@Scenario@@')
assert body[step + 1].op_str == 'eax, 1' and body[step + 1].mnemonic == 'cmp'
reject(step + 1, 'ja')
distance, = sites(body, '?IsWithinDistInMap@WorldObject@@')
loads = [entry for entry in body[distance - 6:distance] if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP]
assert len(loads) == 1
address = loads[0].address + loads[0].size + loads[0].operands[1].mem.disp
assert struct.unpack_from('<f', native['binary'], native['offset'](address))[0] == 20.0

los = '?MoveInLineOfSight@npc_travar_firstAI@npc_travar_first@@'
decoded = instructions(los)
arm, = sites(decoded, '?AddDelayedEvent@FunctionProcessor@@')
gate, = sites(decoded, helper)
assert decoded[gate + 1].op_str == 'al, al' and decoded[gate + 2].mnemonic == 'je'
assert not reaches(decoded, decoded[gate + 2].operands[0].imm, decoded[arm].address)
latch = next(index for index, entry in enumerate(decoded) if entry.op_str == 'byte ptr [rcx + 0x58], 0')
assert decoded[latch + 1].mnemonic == 'jne' and not reaches(decoded, decoded[latch + 1].operands[0].imm, decoded[arm].address)
assert any(entry.op_str == 'edx, 0x1388' for entry in decoded[arm - 3:arm])
guid, = sites(decoded, '?GetGuidValue@Object@@')
assert gate < guid < arm
assert any(entry.mnemonic == 'xor' and entry.op_str == 'edx, edx' for entry in decoded[guid - 3:guid]), 'GUID copy is not native OBJECT_FIELD_GUID0'
assert any(entry.mnemonic == 'movups' and entry.op_str == 'xmm0, xmmword ptr [rax]' for entry in decoded[guid + 1:arm])
assert any(entry.op_str == 'xmmword ptr [rsp + 0x40], xmm0' for entry in decoded[guid + 1:arm])
assert any(entry.op_str == 'qword ptr [rsp + 0x38], rdi' for entry in decoded[guid + 1:arm])
closures = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'MoveInLineOfSight@npc_travar_firstAI@npc_travar_first@@' in name]
assert len(closures) == 1
closure = closures[0]
installed = []
for entry in decoded:
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            installed.append(address)
assert len(installed) == 1, 'Timer does not install checked GUID closure'
delayed = instructions(closure)
accessor = '?GetPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVWorldObject@@VObjectGuid@@@Z'
lookup, = sites(delayed, accessor)
revalidate, = sites(delayed, helper)
move, = sites(delayed, '?MovePoint@MotionMaster@@')
credit, = sites(delayed, '?KilledMonsterCredit@Player@@')
assert lookup < revalidate < move < credit
assert any(entry.op_str == 'xmm0, xmmword ptr [rcx + 0x10]' for entry in delayed[:lookup])
assert any(entry.op_str == 'rcx, qword ptr [rcx + 0x20]' for entry in delayed[:lookup])
assert delayed[lookup + 2].op_str == 'rcx, rax' and delayed[lookup + 3].op_str == 'rdi, rax'
assert delayed[revalidate + 1].op_str == 'al, al' and delayed[revalidate + 2].mnemonic == 'jne'
assert not reaches(delayed, delayed[revalidate + 3].address, delayed[credit].address)
assert any(entry.op_str == 'byte ptr [rax + 0x58], 0' for entry in delayed[revalidate + 3:move])
assert any(entry.op_str == 'edx, 0x19fb0' for entry in delayed[credit - 5:credit])
assert any(entry.op_str == 'rcx, rdi' for entry in delayed[credit - 5:credit])
assert not sites(delayed, '?FindNearestPlayer@WorldObject@@'), 'Delayed recipient substitutes a nearby player'
selected = []
for entry in instructions('?GetAI@npc_travar_first@@'):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x270)[0] == native['symbol'](los):
            selected.append(address)
assert len(selected) == 1, 'Factory does not select checked LOS at native slot0x270'
safe = instructions('?MoveInLineOfSight_Safe@CreatureAI@@')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x270 for entry in safe)
lookup_body = instructions(accessor)
assert native['symbol']('?IsInWorld@Object@@') in native['calls'](accessor)
assert native['symbol']('?IsPreDelete@Object@@') in native['calls'](accessor)
assert any(entry.op_str == 'qword ptr [rbx + 8], rdi' for entry in lookup_body)
print('PASS: selected native entrance LOS slot0x270 arms5s only for live Holy1092/steps0-1/active42120/spatial actor context, captures full GUID plus AI rather than Player pointer, resolves native map/world/pre-delete recipient and revalidates before106416/MovePoint. Failure clears only intro latch for retry; no nearest-player substitution. Later story/group/client traversal remain separate.')
