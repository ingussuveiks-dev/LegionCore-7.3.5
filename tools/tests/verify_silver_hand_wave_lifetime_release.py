"""Verify last Travard's selected AI-owned wave timer in the canonical PE."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp')
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


def leaf(prefix, size):
    address = native['symbol'](prefix)
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    body = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + size], address))
    assert sum(entry.size for entry in body) == size
    return body


los_name = '?MoveInLineOfSight@npc_travar_lastAI@npc_travar_last@@'
lifecycle_name = '?UpdateLifecycle@npc_travar_lastAI@'
destructor_name = '??_Gnpc_travar_lastAI@'
factory = instructions('?GetAI@npc_travar_last@@')
los = instructions(los_name)
selected = []
for entry in factory:
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if slots[0x270 // 8] == native['symbol'](los_name):
            selected.append(slots)
slots, = selected
assert slots[0x110 // 8] == native['symbol'](lifecycle_name) and slots[0] == native['symbol'](destructor_name)
construct, = sites(factory, '??0FunctionProcessor@@')
assert factory[construct - 1].op_str == 'rcx, [rdi + 0x48]'
assert all(any(entry.op_str == operands for entry in factory) for operands in
           ('ecx, 0xd8', 'word ptr [rdi + 0x38], 0', 'dword ptr [rdi + 0x3c], 0x3e8', 'byte ptr [rdi + 0x40], 0'))
arm, = sites(los, '?AddDelayedEvent@FunctionProcessor@@')
assert any(entry.op_str == 'rcx, [rsi + 0x48]' for entry in los[arm - 10:arm])
assert los[arm - 1].op_str == 'edx, 0x1388'
assert not sites(los, '?AddDelayedEvent@Unit@@')
assert any(entry.op_str == 'byte ptr [rdx + 0x16], 4' for entry in los[:arm])
assert any(entry.op_str == 'word ptr [rsi + 0x38], 0x101' for entry in los[:arm])
assert any(entry.op_str == 'qword ptr [rsp + 0x68], rsi' for entry in los[arm - 10:arm])
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'MoveInLineOfSight@npc_travar_lastAI@npc_travar_last@@' in name]
tables = []
for index, entry in enumerate(los[:arm]):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            assert los[index + 1].op_str == 'qword ptr [rsp + 0x60], rax'
            tables.append(address)
assert len(tables) == 1
callback = leaf(closure, 9)
lambda_name, = [name for name in native['symbols'] if name.startswith('??R<lambda_1>@?6??MoveInLineOfSight@npc_travar_lastAI@')]
assert [(entry.mnemonic, entry.op_str) for entry in callback[:1]] == [('add', 'rcx, 8')]
assert callback[1].mnemonic == 'jmp' and callback[1].operands[0].imm == native['symbol'](lambda_name)
wave = instructions(lambda_name)
spawn, = sites(wave, '?SummonAdds@npc_travar_lastAI@')
wave_helper = '?CanStartWave@SilverHandFinalEntry@@'
gate, = sites(wave, wave_helper)
assert [entry.op_str for entry in wave[gate - 3:gate]] == [
    'rbx, rcx', 'rcx, qword ptr [rcx]', 'rcx, qword ptr [rcx + 0x20]']
assert [entry.op_str for entry in wave[gate + 1:gate + 3]] == ['rcx, qword ptr [rbx]', 'al, al']
assert wave[gate + 3].mnemonic == 'jne' and wave[gate + 3].operands[0].imm == wave[spawn - 1].address
assert wave[spawn - 1].op_str == 'dl, 1'
cancel = gate + 4
assert [entry.op_str for entry in wave[cancel:cancel + 3]] == [
    'byte ptr [rcx + 0x38], al', 'rax, qword ptr [rbx]', 'byte ptr [rax + 0x39], 0']
assert wave[spawn - 2].mnemonic == 'jmp' and wave[spawn - 2].operands[0].imm == wave[spawn + 1].address
wave_policy = instructions(wave_helper)
deny = next(entry.address for entry in wave_policy if entry.mnemonic == 'xor' and entry.op_str == 'al, al')
assert any(entry.mnemonic == 'mov' and entry.op_str == 'al, 1' for entry in wave_policy)
for mnemonic, operands in (('xor', 'al, al'), ('mov', 'al, 1')):
    result, = [index for index, entry in enumerate(wave_policy) if entry.mnemonic == mnemonic and entry.op_str == operands]
    assert [(entry.mnemonic, entry.op_str) for entry in wave_policy[result + 1:result + 4]] == [
        ('add', 'rsp, 0x20'), ('pop', 'rbx'), ('ret', '')]
for operands, branch in (('rcx, rcx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('eax, 0x1a119', 'jne'), ('dword ptr [rbx + 0xe0], 0x603', 'jne'),
                         ('qword ptr [rbx + 8], 0', 'je'), ('rax, rax', 'je'),
                         ('eax, 0x444', 'jne'), ('eax, 9', 'jne')):
    position, = [index for index, entry in enumerate(wave_policy) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operands]
    assert wave_policy[position + 1].mnemonic == branch and wave_policy[position + 1].operands[0].imm == deny
for method, branch in (('?IsInWorld@Object@@', 'je'), ('?IsPreDelete@Object@@', 'jne')):
    position, = sites(wave_policy, method)
    assert wave_policy[position + 1].op_str == 'al, al'
    assert wave_policy[position + 2].mnemonic == branch and wave_policy[position + 2].operands[0].imm == deny
assert any(entry.op_str == 'rbx, rcx' for entry in wave_policy[:sites(wave_policy, '?IsInWorld@Object@@')[0]])
identity, = sites(wave_policy, '?GetUInt32Value@Object@@')
assert [entry.op_str for entry in wave_policy[identity - 2:identity]] == ['edx, 9', 'rcx, rbx']
scenario, = sites(wave_policy, '?GetScenario@ScenarioMgr@@')
assert wave_policy[scenario - 2].op_str == 'edx, dword ptr [rbx + 0x3b8]'
assert sites(wave_policy, '?GetScenarioId@Scenario@@') and sites(wave_policy, '?GetCurrentStep@Scenario@@')
for method in ('?FindNearestPlayer@WorldObject@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddDelayedEvent@FunctionProcessor@@'):
    assert not sites(wave, method) and not sites(wave_policy, method)
message = b'Silver Hand ritual: pending first wave context changed; rearming entry without spawning'
assert any(entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM
           and entry.operands[1].mem.base == X86_REG_RIP
           and native['binary'].startswith(message, native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))
           for entry in wave[cancel:])
lifecycle = leaf(lifecycle_name, 9)
assert lifecycle[0].mnemonic == 'add' and lifecycle[0].op_str == 'rcx, 0x48'
assert lifecycle[1].mnemonic == 'jmp' and native['resolve'](lifecycle[1].operands[0].imm) == native['symbol']('?Update@FunctionProcessor@@')
destroy = instructions(destructor_name)
release, = sites(destroy, '??1FunctionProcessor@@')
assert destroy[release - 1].mnemonic == 'lea' and destroy[release - 1].op_str == 'rcx, [rdi + 0x48]'
creature = instructions('?Update@Creature@@')
ticks = [index for index, entry in enumerate(creature) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x110]
assert len(ticks) == 2
assert any(entry.op_str == 'byte ptr [rsi + 0x3a9d], 1' for entry in creature[ticks[0] - 10:ticks[0]])
assert creature[ticks[1] + 1].op_str == 'byte ptr [rsi + 0x3a9d], 0'
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c8]' for entry in creature[ticks[0] - 8:ticks[0]])
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c0]' for entry in creature[ticks[0]:ticks[1]])
assert min(sites(creature, '?Update@Unit@@')) > ticks[1]
print('PASS: canonical last Travard factory selects LOS0x270, lifecycle0x110 and destructor0; actual LOS captures AI and queues5000ms at AI+0x48. Selected9-byte callable tail-dispatches lambda through actual outlined CanStartWave with live106777/map1539/nondeleted/alive/map/scenario1092/exact9 gates before sole SummonAdds(1); false result clears only intro38/criteria39 and retains diagnostic, no credit or self-requeue. Selected9-byte lifecycle drains same queue under primary/disabled lock; destructor releases it. Wave recovery/channel/quest epochs and client traversal remain separate.')
