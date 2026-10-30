"""Verify the selected entrance AI and its GUID-resolved delayed recipient."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_collapse_release.py')))
native, instructions = previous['native'], previous['instructions']
callback_slot = runpy.run_path(str(Path(__file__).with_name('pe_creature_ai_slots.py')))['callback_slot']
los_slot = callback_slot(native, '?MoveInLineOfSight@SmartAI@@')
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
assert any(entry.op_str == 'xmmword ptr [rsp + 0x30], xmm0' for entry in decoded[guid + 1:arm])
assert any(entry.op_str == 'qword ptr [rsp + 0x48], rdi' for entry in decoded[guid + 1:arm])
assert any(entry.op_str == 'xmmword ptr [rsp + 0x50], xmm0' for entry in decoded[guid + 1:arm])
assert any(entry.op_str == 'qword ptr [rsp + 0x60], rbx' for entry in decoded[guid + 1:arm])
assert any(entry.op_str == 'rcx, [rdi + 0xf0]' for entry in decoded[guid + 1:arm])
talk, = sites(decoded, '?Talk@CreatureAI@@QEAAXE')
epoch = next(index for index, entry in enumerate(decoded) if entry.op_str == 'rbx, qword ptr [rdi + 0x170]')
assert guid < epoch < talk < arm
assert any(entry.op_str == 'byte ptr [rdi + 0x178], 1' for entry in decoded[gate:guid])
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
assert any(entry.op_str == 'xmm0, xmmword ptr [rbx + 0x10]' for entry in delayed[:lookup])
assert any(entry.op_str == 'rcx, qword ptr [rcx + 0x20]' for entry in delayed[:lookup])
pending = next(index for index, entry in enumerate(delayed) if entry.op_str == 'byte ptr [rcx + 0x178], 0' and entry.mnemonic == 'cmp')
generation = next(index for index, entry in enumerate(delayed) if entry.op_str == 'qword ptr [rbx + 0x20], rax' and entry.mnemonic == 'cmp')
assert pending < generation < lookup
assert any(entry.op_str == 'rax, qword ptr [rcx + 0x170]' for entry in delayed[pending:generation])
for position, branch in ((pending, 'je'), (generation, 'jne')):
    assert delayed[position + 1].mnemonic == branch and not reaches(delayed, delayed[position + 1].operands[0].imm, delayed[credit].address)
assert any(entry.op_str == 'byte ptr [rcx + 0x178], 0' and entry.mnemonic == 'mov' for entry in delayed[generation:lookup])
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
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + los_slot * 8)[0] == native['symbol'](los):
            selected.append(address)
assert len(selected) == 1, 'Factory does not select checked native LOS slot'
safe = instructions('?MoveInLineOfSight_Safe@CreatureAI@@')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == los_slot * 8 for entry in safe)
lookup_body = instructions(accessor)
assert native['symbol']('?IsInWorld@Object@@') in native['calls'](accessor)
assert native['symbol']('?IsPreDelete@Object@@') in native['calls'](accessor)
assert any(entry.op_str == 'qword ptr [rbx + 8], rdi' for entry in lookup_body)
slots = struct.unpack_from('<80Q', native['binary'], native['offset'](selected[0]))
reset_name = '?Reset@npc_travar_firstAI@'
lifecycle_name = '?UpdateLifecycle@npc_travar_firstAI@'
destructor_name = '??_Gnpc_travar_firstAI@'
assert slots[0x28 // 8] == native['symbol'](reset_name) and slots[0x110 // 8] == native['symbol'](lifecycle_name)
assert slots[0] == native['symbol'](destructor_name)
factory = instructions('?GetAI@npc_travar_first@@')
constructors = sites(factory, '??0FunctionProcessor@@')
assert len(constructors) == 2
assert [factory[position - 1].op_str for position in constructors] == ['rcx, [rsi + 0x60]', 'rcx, [rsi + 0xf0]']
assert any(entry.op_str == 'qword ptr [rsi + 0x170], rbp' for entry in factory)
assert any(entry.op_str == 'byte ptr [rsi + 0x178], bpl' for entry in factory)
assert any(entry.mnemonic == 'xor' and entry.op_str == 'ebp, ebp' for entry in factory)
reset_body = instructions(reset_name)
guard = next(index for index, entry in enumerate(reset_body) if entry.op_str == 'byte ptr [rbx + 0x178], 0' and entry.mnemonic == 'cmp')
assert reset_body[guard + 2].mnemonic == 'je'
assert [entry.op_str for entry in reset_body[guard + 3:guard + 6]] == [
    'qword ptr [rbx + 0x170]', 'byte ptr [rbx + 0x178], 0', 'byte ptr [rbx + 0x58], 0']
assert reset_body[guard + 3].mnemonic == 'inc'
assert not reaches(reset_body, reset_body[guard + 2].operands[0].imm, reset_body[guard + 3].address)
assert not sites(reset_body, '?KillAllFunctions@FunctionProcessor@@') and not sites(reset_body, '?Clear@ObjectGuid@@')
lifecycle_body = instructions(lifecycle_name)
recover, = sites(lifecycle_body, '?Recover@SilverHandCompanion@@')
assert any(entry.op_str == 'rcx, [rbx + 0xf0]' for entry in lifecycle_body[recover + 1:])
assert lifecycle_body[-1].mnemonic == 'jmp' and native['resolve'](lifecycle_body[-1].operands[0].imm) == native['symbol']('?Update@FunctionProcessor@@')
destroy_body = instructions(destructor_name)
destructors = sites(destroy_body, '??1FunctionProcessor@@')
assert len(destructors) == 2 and destroy_body[destructors[0] - 1].op_str == 'rcx, 0xf0'
assert destroy_body[destructors[1] - 1].op_str == 'rcx, [rdi + 0x60]'
creature = instructions('?Update@Creature@@')
ticks = [index for index, entry in enumerate(creature) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x110]
assert len(ticks) == 2
assert any(entry.op_str == 'byte ptr [rsi + 0x3a9d], 1' for entry in creature[ticks[0] - 10:ticks[0]])
assert creature[ticks[1] + 1].op_str == 'byte ptr [rsi + 0x3a9d], 0'
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c8]' for entry in creature[ticks[0] - 8:ticks[0]])
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c0]' for entry in creature[ticks[0]:ticks[1]])
assert min(sites(creature, '?Update@Unit@@')) > ticks[1]
motion = instructions('?MovementInform@npc_travar_firstAI@')
fight_spawn = '?SpawnFirstFight@npc_travar_firstAI@'
assert sites(motion, fight_spawn)
fight = instructions(fight_spawn)
fight_arm, = sites(fight, '?AddDelayedEvent@FunctionProcessor@@')
assert any(entry.op_str == 'edx, 0x7d0' for entry in fight[fight_arm - 3:fight_arm])
assert any(entry.op_str == 'rcx, [rbx + 0xf0]' for entry in fight[fight_arm - 10:fight_arm])
fight_tables = [entry.address + entry.size + entry.operands[1].mem.disp for entry in fight[fight_arm - 10:fight_arm]
               if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP]
assert len(fight_tables) == 1
fight_target = struct.unpack_from('<Q', native['binary'], native['offset'](fight_tables[0]) + 0x10)[0]
assert any(name.startswith('?_Do_call@') and 'SpawnFirstFight@npc_travar_firstAI' in name
           and address == fight_target for name, address in native['symbols'].items())
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
fight_body = list(decoder.disasm(native['binary'][native['offset'](fight_target):native['offset'](fight_target) + 40], fight_target))
epoch = next(i for i, entry in enumerate(fight_body) if entry.op_str == 'rax, qword ptr [rdx + 0x190]')
assert fight_body[epoch + 1].op_str == 'qword ptr [rcx + 0x10], rax' and fight_body[epoch + 2].mnemonic == 'jne'
ready = next(i for i, entry in enumerate(fight_body) if entry.op_str == 'byte ptr [rdx + 0x5c], 1')
assert all(entry.operands[0].imm > fight_body[ready].address for entry in fight_body[:ready] if entry.mnemonic in ('je', 'jne'))
print('PASS: selected first LOS at the resolved native slot owns5s queue atAI+0xf0; original full128-bit GUID/64-bit Reset generation snapshot precedes immediate Talk0. Native locked primary/disabled lifecycle0x110 retains companion recovery then services entry; selected destructor drops both queues. Pending-only Reset invalidates stale entry without clearing accepted POINT1 token or replaying route. Closure consumes matching epoch before native original lookup/live1092/0..1/20yd, Talk1/full arrival token/MovePoint/106416. Actual MovementInform calls the owned-fight spawn helper, which queues the original2s marker onAI+0xf0; its selected closure checks the active fight epoch before writing firstfight+0x5c. Later story/native counters/client traversal remain separate.')
