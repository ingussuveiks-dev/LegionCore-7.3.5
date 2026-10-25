"""Verify selected rescue post-heal callback, native queue and scoped recipient."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_aberration_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
sites, reaches = previous['sites'], previous['reaches']
helper = '?CanCredit@SilverHandRescue@@'
body = instructions(helper)
deny = next(entry.address for index, entry in enumerate(body[:-1]) if entry.op_str == 'rbx, qword ptr [rsp + 0x40]'
            and body[index + 1].mnemonic == 'xor' and body[index + 1].op_str == 'al, al')


def reject(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == deny


for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'), ('byte ptr [rbx + 0x4ff0], 1', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('dword ptr [rdi + 0xa18], 0', 'jne'), ('dword ptr [rdi + 0xe0], 0x603', 'jne'),
                         ('rax, rax', 'je'), ('rax, qword ptr [rdi + 8]', 'jne'),
                         ('dword ptr [rbx + 0x3b8], eax', 'jne')):
    position = next(index for index, entry in enumerate(body) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test'))
    reject(position, branch)
for method, branch, count in (('?IsInWorld@Object@@', 'je', 2), ('?IsPreDelete@Object@@', 'jne', 2),
                              ('?CanContact@Player@@', 'je', 1), ('?InSamePhase@WorldObject@@', 'je', 1),
                              ('?IsWithinDistInMap@WorldObject@@', 'je', 1), ('?GetQuestRewardStatus@Player@@', 'jne', 1)):
    positions = sites(body, method)
    assert len(positions) == count
    for position in positions:
        assert body[position + 1].op_str == 'al, al'
        reject(position + 1, branch)
for method, operands in (('?GetByteValue@Object@@', 'al, 2'), ('?GetQuestStatus@Player@@', 'eax, 3'),
                         ('?GetScenarioId@Scenario@@', 'eax, 0x444'), ('?GetCurrentStep@Scenario@@', 'eax, 7')):
    position, = sites(body, method)
    assert body[position + 1].op_str == operands
    reject(position + 1, 'jne')
entries = sites(body, '?GetUInt32Value@Object@@')
assert len(entries) == 3
for position, identity in zip(entries, (106933, 106721, 106722)):
    assert body[position - 2].op_str == 'edx, 9' and body[position - 1].op_str == 'rcx, rdi'
    assert body[position + 1].op_str == f'eax, {identity:#x}'
assert body[entries[0] + 2].mnemonic == body[entries[1] + 2].mnemonic == 'je'
assert body[entries[0] + 2].operands[0].imm == body[entries[1] + 2].operands[0].imm == body[entries[2] + 3].address
reject(entries[2] + 1, 'jne')
for method in ('?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@'):
    position, = sites(body, method)
    assert any(entry.op_str == 'edx, 0xa488' for entry in body[position - 3:position])
scenario, = sites(body, '?GetScenario@ScenarioMgr@@')
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
floats = previous['previous']['floats']
distance, = sites(body, '?IsWithinDistInMap@WorldObject@@')
assert floats(body[distance - 7:distance]) == [100.0]

select = '?SelectPlayer@SilverHandRescue@@'
body = instructions(select)
gates = sites(body, helper)
assert len(gates) == 2
assert body[gates[0] + 1].op_str == 'al, al' and body[gates[0] + 2].mnemonic == 'je'
assert body[gates[0] + 3].op_str == 'rax, rbx'
empty = next(index for index, entry in enumerate(body) if entry.mnemonic == 'xor' and entry.op_str == 'eax, eax')
for index in range(gates[0] + 3, gates[1]):
    if body[index].mnemonic == 'je':
        assert body[index].operands[0].imm == body[empty].address
assert any(entry.op_str == 'rax, qword ptr [rax + 0x768]' for entry in body[gates[0]:gates[1]])
assert body[gates[1] - 2].op_str == 'rcx, qword ptr [rbx + 0x20]' and body[gates[1] - 1].op_str == 'rdx, rdi'
assert body[gates[1] + 1].op_str == 'al, al' and body[gates[1] + 2].mnemonic == 'jne'
found = next(index for index, entry in enumerate(body) if entry.op_str == 'rax, qword ptr [rbx + 0x20]')
assert body[gates[1] + 2].operands[0].imm == body[found].address
assert body[gates[1] + 3].op_str == 'rbx, qword ptr [rbx + 8]'
assert any(entry.mnemonic == 'jne' and entry.operands[0].imm == body[gates[1] - 2].address for entry in body[gates[1]:empty])

callback = '?HealReceived@npc_silverhand_eventsAI@npc_silverhand_events@@'
body = instructions(callback)
owner, = sites(body, '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
valid, = sites(body, select)
guid, = sites(body, '?GetGuidValue@Object@@')
queue, = sites(body, '?AddDelayedEvent@FunctionProcessor@@')
assert owner < valid < guid < queue
assert body[valid + 1].op_str == 'rax, rax' and body[valid + 2].mnemonic == 'je'
assert not reaches(body, body[valid + 2].operands[0].imm, body[queue].address)
for operands, branch in (('byte ptr [rcx + 0x38], 0', 'jne'), ('dword ptr [r8], 0', 'je'),
                         ('rdx, rdx', 'je'), ('qword ptr [rbx + 8], rax', 'jne')):
    position = next(index for index, entry in enumerate(body) if entry.op_str == operands)
    assert body[position + 1].mnemonic == branch and not reaches(body, body[position + 1].operands[0].imm, body[queue].address)
assert body[queue - 1].op_str == 'edx, edx' and body[queue - 1].mnemonic == 'xor'
assert any(entry.op_str == 'rcx, [rdi + 0x48]' for entry in body[guid:queue])
for operands in ('xmm0, xmmword ptr [rax]', 'qword ptr [rsp + 0x48], xmm0', 'qword ptr [rsp + 0x50], rax',
                 'rax, qword ptr [rdi + 0x40]', 'qword ptr [rsp + 0x58], rax', 'qword ptr [rsp + 0x60], rdi'):
    assert any(entry.op_str == operands for entry in body[guid:queue])
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'HealReceived@npc_silverhand_eventsAI' in name]
tables = [entry.address + entry.size + entry.operands[1].mem.disp for entry in body
          if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP
          and struct.unpack_from('<Q', native['binary'], native['offset'](entry.address + entry.size + entry.operands[1].mem.disp) + 0x10)[0] == native['symbol'](closure)]
assert len(tables) == 1
assert not sites(body, '?GetHealthPct@Unit@@') and not sites(body, '?UpdateAchievementCriteria@Player@@')

body = instructions(closure)
lookup, = sites(body, '?GetPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVWorldObject@@VObjectGuid@@@Z')
valid, = sites(body, select)
health, = sites(body, '?GetHealthPct@Unit@@')
removals = sites(body, '?RemoveAura@Unit@@QEAAXI')
despawn, = sites(body, '?DespawnOrUnsummon@Creature@@')
move, = sites(body, '?MovePoint@MotionMaster@@')
credit, = sites(body, '?UpdateAchievementCriteria@Player@@')
assert health < lookup < valid < removals[0] < removals[1] < despawn < move < credit
for operands in ('byte ptr [rcx + 0x38], 0', 'qword ptr [rdi + 0x18], rax'):
    position = next(index for index, entry in enumerate(body) if entry.op_str == operands)
    assert body[position + 1].mnemonic == 'jne' and not reaches(body, body[position + 1].operands[0].imm, body[credit].address)
assert floats(body[health:lookup]) == [90.0]
threshold = next(index for index, entry in enumerate(body) if entry.mnemonic == 'comiss' and entry.op_str == 'xmm1, xmm0')
assert body[threshold + 1].mnemonic == 'ja' and not reaches(body, body[threshold + 1].operands[0].imm, body[credit].address)
assert any(entry.op_str == 'xmm0, xmmword ptr [rdi + 8]' for entry in body[health:lookup])
assert body[valid + 1].op_str == 'rbx, rax' and body[valid + 2].op_str == 'rax, rax'
assert body[valid + 3].mnemonic == 'je' and not reaches(body, body[valid + 3].operands[0].imm, body[credit].address)
assert any(entry.op_str == 'byte ptr [rcx + 0x38], 1' for entry in body[valid:removals[0]])
assert any(entry.mnemonic == 'and' and entry.op_str == 'dword ptr [rdx + 0x3140], 0xfffffff7' for entry in body[valid:removals[0]])
for position, identity in zip(removals, (211699, 211698)):
    assert any(entry.op_str == f'edx, {identity:#x}' for entry in body[position - 10:position])
assert any(entry.op_str == 'edx, 0xbb8' for entry in body[despawn - 5:despawn])
for operands in ('eax, 0x1a1b5', 'edx, 0x5c', 'r8d, 0xc6e1'):
    assert any(entry.op_str == operands for entry in body[move:credit])
assert any(entry.mnemonic == 'setne' and entry.op_str == 'r8b' for entry in body[move:credit])
assert not sites(body, '?FindNearestPlayer@WorldObject@@') and not sites(body, '?AddItem@Player@@')

factory = instructions('?GetAI@npc_silverhand_events@@')
selected = []
reset = '?Reset@npc_silverhand_eventsAI@npc_silverhand_events@@'
lifecycle = '?UpdateLifecycle@npc_silverhand_eventsAI@npc_silverhand_events@@'
destructor = '??_Gnpc_silverhand_eventsAI@npc_silverhand_events@@'
for entry in factory:
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if (slots[0xb0 // 8] == native['symbol'](callback) and slots[0x28 // 8] == native['symbol'](reset)
                and slots[0x110 // 8] == native['symbol'](lifecycle) and slots[0] == native['symbol'](destructor)):
            selected.append(address)
assert len(selected) == 1
assert any(entry.op_str == 'qword ptr [rdi + 0x40], 0' for entry in factory)
assert any(entry.op_str == 'byte ptr [rdi + 0x38], 0' for entry in factory)
construct, = sites(factory, '??0FunctionProcessor@@')
assert factory[construct - 1].op_str == 'rcx, [rdi + 0x48]'
address = native['offset'](native['symbol'](reset))
assert native['binary'][address:address + 5] == bytes.fromhex('48ff4140c3')
address = native['symbol'](lifecycle)
leaf = native['binary'][native['offset'](address):native['offset'](address) + 9]
assert leaf[:5] == bytes.fromhex('4883c148e9')
assert address + 9 + struct.unpack_from('<i', leaf, 5)[0] == native['symbol']('?Update@FunctionProcessor@@')
destroy = instructions(destructor)
release, = sites(destroy, '??1FunctionProcessor@@')
assert destroy[release - 1].op_str == 'rcx, 0x48'
creature = instructions('?Update@Creature@@')
ticks = [index for index, entry in enumerate(creature) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x110]
assert len(ticks) == 2
assert any(entry.op_str == 'byte ptr [rsi + 0x3a9d], 1' for entry in creature[ticks[0] - 10:ticks[0]])
assert creature[ticks[1] + 1].op_str == 'byte ptr [rsi + 0x3a9d], 0'
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c8]' for entry in creature[ticks[0] - 8:ticks[0]])
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c0]' for entry in creature[ticks[0]:ticks[1]])
assert min(sites(creature, '?Update@Unit@@')) > ticks[1]
heal = instructions('?DealHeal@Unit@@')
received, = [index for index, entry in enumerate(heal) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xb0]
done, = [index for index, entry in enumerate(heal) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xb8]
apply, = sites(heal, '?ModifyHealth@Unit@@')
assert received < done < apply
enqueue = instructions('?AddDelayedEvent@FunctionProcessor@@')
insert, = sites(enqueue, '??$insert@U?$pair@_KV?$function')
assert any(entry.op_str == 'r14, qword ptr [rcx]' for entry in enqueue[:insert])
assert any(entry.op_str == 'rax, [r14 + rsi]' for entry in enqueue[:insert])
assert any(entry.op_str == 'rcx, [rdi + 0x18]' for entry in enqueue[insert - 4:insert])
assert not any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x10 for entry in enqueue)
update = instructions('?Update@FunctionProcessor@@')
drain, = sites(update, '?AddFunctionsFromQueue@FunctionProcessor@@')
invoke, = [index for index, entry in enumerate(update) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x10]
assert drain < invoke
assert native['symbol']('?Update@FunctionProcessor@@') in native['calls']('?Update@Unit@@')
print('PASS: selected rescue HealReceived0xb0 queues AI-owned zero-delay FunctionProcessor at+0x48 with full128-bit Player GUID/64-bit reset generation/AI, never tests pre-heal HP. Native locked primary/disabled Lifecycle0x110 runs it independently of stunned combat AI; selected destructor destroys its queue on replacement. Selected closure checks actual90pct after HealDone/ModifyHealth, revalidates Holy1092/exact7 participant and latches before original stun/aura cleanup,3000ms despawn, MovePoint0 and one92/50913-or50914. Reset leaf increments generation without clearing completed latch. Shared criteria remain native; no fabricated heal/item/party credit. Real spell targeting, persisted2+2/group counters and rendered client behavior remain separate.')
