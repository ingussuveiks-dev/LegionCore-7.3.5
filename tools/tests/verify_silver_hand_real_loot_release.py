"""Check actual completed-loot, scene retry and guarded return consumers."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP, X86_REG_RSP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_pickup_admission_release.py')))
native = previous['native']
instructions = previous['instructions']
callback = '?OnLootStateChanged@go_art_silverhand@@'
body = instructions(callback)


def sites(decoded, prefix):
    return [index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


def reaches(decoded, start, destination):
    positions = {entry.address: index for index, entry in enumerate(decoded)}
    pending = [start]
    visited = set()
    while pending:
        address = pending.pop()
        if address == destination:
            return True
        if address in visited or address not in positions:
            continue
        visited.add(address)
        entry = decoded[positions[address]]
        if entry.mnemonic in ('ret', 'int3'):
            continue
        if entry.mnemonic.startswith('j'):
            if entry.operands[0].type == CS_OP_IMM:
                pending.append(entry.operands[0].imm)
            if entry.mnemonic == 'jmp':
                continue
        pending.append(entry.address + entry.size)
    return False


cast, = sites(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@')
owned, = sites(body, '?GetItemByEntry@Player@@')
assert all(any(entry.op_str == value for entry in body[owned - 4:owned]) for value in ('edx, 0x1f737', 'r8d, r8d'))
missing = next(index for index in range(owned + 1, owned + 5) if body[index].op_str == 'rax, rax' and body[index].mnemonic == 'test')
assert body[missing + 1].mnemonic == 'je' and not reaches(body, body[missing + 1].operands[0].imm, body[cast].address)
assert all(any(entry.op_str == value for entry in body[cast - 7:cast]) for value in ('r8d, 0x33b7e', 'r9b, 1', 'rdx, rbx', 'rcx, rbx'))
scene_checks = sites(body, '?HasSceneStatus@Player@@')
assert len(scene_checks) == 3 and owned < scene_checks[0] < scene_checks[1] < scene_checks[2] < cast
assert all(any(entry.op_str == 'edx, 0x4f3' for entry in body[position - 4:position]) for position in scene_checks)
assert body[scene_checks[1] + 1].op_str == 'al, al' and body[scene_checks[1] + 2].mnemonic == 'jne'
assert reaches(body, body[scene_checks[1] + 2].operands[0].imm, body[cast].address)
active_aura, = sites(body, '?HasAura@Unit@@')
assert scene_checks[1] < active_aura < scene_checks[2]
assert any(entry.op_str == 'edx, 0x33b7e' for entry in body[active_aura - 7:active_aura])
assert body[active_aura + 1].op_str == 'al, al' and body[active_aura + 2].mnemonic == 'jne'
assert not reaches(body, body[active_aura + 2].operands[0].imm, body[cast].address), 'Active scene can restart from repeated completed-state notification'
assert reaches(body, body[active_aura + 3].address, body[cast].address), 'Expired/cancelled aura leaves a permanent launch-marker block'
equipped, = sites(body, '?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z')
assert body[equipped + 1].op_str == 'rax, r14' and body[equipped + 2].mnemonic == 'je'
assert not reaches(body, body[equipped + 2].operands[0].imm, body[cast].address), 'Already complete equipped claim replays progression'
timer_call, = sites(body, '?AddDelayedEvent@FunctionProcessor@@')
result = next(index for index in range(cast + 1, cast + 5) if body[index].mnemonic == 'test' and body[index].op_str == 'eax, eax')
assert body[result + 1].mnemonic == 'je' and reaches(body, body[result + 1].operands[0].imm, body[timer_call].address)
assert not reaches(body, body[result + 2].address, body[timer_call].address), 'Rejected scene can schedule return'
reopen, = sites(body, '?SetLootState@GameObject@@')
assert reopen > cast and any(entry.op_str == 'edx, 1' for entry in body[reopen - 4:reopen])
assert any(entry.op_str == 'r8d, r8d' for entry in body[reopen - 4:reopen])
assert native['symbol']('?SetLootState@GameObject@@') in native['calls']('?OnGossipHello@go_art_silverhand@@'), 'Owned-copy retry bypasses native state dispatch'
assert native['symbol']('?OnGameObjectLootStateChanged@ScriptMgr@@') in native['calls']('?SetLootState@GameObject@@')
assert any(entry.op_str == 'edx, 0x2ee0' for entry in body[timer_call - 4:timer_call]), 'Original12000ms request changed'
matches = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'go_art_silverhand@@' in name]
assert len(matches) == 1
timer = matches[0]
installed = []
for position, entry in enumerate(body):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](timer):
            installed.append(position)
assert len(installed) == 1, 'Callback does not install checked return closure'
vtable_store = body[installed[0] + 1]
assert vtable_store.mnemonic == 'mov' and vtable_store.operands[0].type == CS_OP_MEM
assert vtable_store.operands[0].mem.base == X86_REG_RSP and vtable_store.op_str.endswith(', rax')
closure_offset = vtable_store.operands[0].mem.disp
assert any(entry.op_str == f'qword ptr [rsp + {closure_offset + 8:#x}], rbx' for entry in body[installed[0] + 2:timer_call])
capture = next(index for index, entry in enumerate(body) if entry.op_str == 'eax, dword ptr [rsi + 0x3b8]')
assert body[capture + 1].op_str == f'dword ptr [rsp + {closure_offset + 16:#x}], eax', 'Return captures a GO instead of Player/instance'
assert any(entry.op_str == f'r8, [rsp + {closure_offset:#x}]' for entry in body[timer_call - 4:timer_call]), 'Checked closure is not the timer argument'
operators = [name for name in native['symbols'] if name.startswith('??R<lambda_1>@?1??OnLootStateChanged@go_art_silverhand@@')]
assert len(operators) == 1
operator = operators[0]
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
address = native['symbol'](timer)
wrapper = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + 9], address))
assert len(wrapper) == 2 and wrapper[0].op_str == 'rcx, 8' and wrapper[0].mnemonic == 'add'
assert wrapper[1].mnemonic == 'jmp' and native['resolve'](wrapper[1].operands[0].imm) == native['symbol'](operator), 'Registered leaf wrapper does not reach captured lambda operator'
decoded = instructions(operator)
assert any(entry.op_str == 'rbx, qword ptr [rcx]' for entry in decoded[:15]), 'Return operator does not use captured Player'
instance_register = next(entry.op_str.split(',')[0] for entry in decoded[:15] if entry.mnemonic == 'mov' and entry.op_str.endswith('dword ptr [rcx + 8]'))
return_cast, = sites(decoded, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@')


def reject(position, branch):
    assert decoded[position + 1].mnemonic == branch
    assert not reaches(decoded, decoded[position + 1].operands[0].imm, decoded[return_cast].address), 'Stale return reaches native scene'


for operands, branch in (('rbx, rbx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('qword ptr [rbx + 8], 0', 'je'), ('dword ptr [rbx + 0xe0], 0x603', 'jne'),
                         (f'dword ptr [rbx + 0x3b8], {instance_register}', 'jne')):
    position = next(index for index, entry in enumerate(decoded[:return_cast]) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test'))
    reject(position, branch)
for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'),
                       ('?GetQuestRewardStatus@Player@@', 'jne'), ('?HasSceneStatus@Player@@', 'je'), ('?IsCompleted@Scenario@@', 'je')):
    position, = sites(decoded, prefix)
    assert decoded[position + 1].op_str == 'al, al'
    reject(position + 1, branch)
klass, = sites(decoded, '?GetByteValue@Object@@')
assert decoded[klass + 1].op_str == 'al, 2'
reject(klass + 1, 'jne')
statuses = sites(decoded, '?GetQuestStatus@Player@@')
assert len(statuses) == 2 and all(any(entry.op_str == 'edx, 0xa488' for entry in decoded[position - 3:position]) for position in statuses)
assert decoded[statuses[0] + 1].op_str == 'eax, 3' and decoded[statuses[0] + 2].mnemonic == 'je'
assert decoded[statuses[1] + 1].op_str == 'eax, 1'
reject(statuses[1] + 1, 'jne')
scene, = sites(decoded, '?HasSceneStatus@Player@@')
assert any(entry.op_str == 'edx, 0x4f3' for entry in decoded[scene - 4:scene])
owned, = sites(decoded, '?GetItemByEntry@Player@@')
assert any(entry.op_str == 'edx, 0x1f737' for entry in decoded[owned - 4:owned])
missing = next(index for index in range(owned + 1, owned + 5) if decoded[index].op_str == 'rax, rax' and decoded[index].mnemonic == 'test')
reject(missing, 'je')
equipped, = sites(decoded, '?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z')
assert decoded[owned + 1].mnemonic == 'mov' and decoded[owned + 1].op_str.endswith(', rax')
weapon_register = decoded[owned + 1].op_str.split(',')[0]
assert decoded[equipped + 1].op_str == f'rax, {weapon_register}'
reject(equipped + 1, 'jne')
scenario, = sites(decoded, '?GetScenario@ScenarioMgr@@')
assert any(entry.op_str == f'edx, {instance_register}' for entry in decoded[scenario - 3:scenario])
missing = next(index for index in range(scenario + 1, scenario + 5) if decoded[index].op_str == 'rax, rax' and decoded[index].mnemonic == 'test')
reject(missing, 'je')
identity, = sites(decoded, '?GetScenarioId@Scenario@@')
assert decoded[identity + 1].op_str == 'eax, 0x444'
reject(identity + 1, 'jne')
assert all(any(entry.op_str == value for entry in decoded[return_cast - 12:return_cast]) for value in ('r8d, 0x33b96', 'r9d, r9d', 'rdx, rcx'))
assert b'return scene 211862 failed with result' in native['binary']
for prefix in (callback, operator):
    targets = native['calls'](prefix)
    for forbidden in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@'):
        assert native['symbol'](forbidden) not in targets, f'Direct item/arrival/progression in {prefix}'
for dependency in ('?LootItemInSlot@Loot@@', '?StoreNewItem@Player@@', '?SendEquipError@Player@@'):
    assert native['symbol'](dependency) in native['calls']('?StoreLootItem@Player@@'), f'Native storage consumer missing: {dependency}'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEAA_NPEAVWorldObject@@AEBV') in native['calls']('?AllowedForPlayer@LootItem@@')
assert native['symbol']('?SetLootState@GameObject@@') in native['calls']('?DoLootRelease@WorldSession@@')
print('PASS: actual stored-state3+carried item authorizes211838, repeated active-aura/already-equipped complete claims stop; expired aura does not leave a permanent launch-marker block, rejected scene reopens native READY without return. Checked installed Player/instance12000ms closure requires live quest/scene-complete/equipped item/completed1092 before211862, no105892 or direct item/progression grant. Native slot/conditions/store/error/release retained. Full late return/client traversal remains separate.')
