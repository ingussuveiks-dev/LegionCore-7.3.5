"""Verify selected instance-owned, full-GUID Blinding Light cast recovery."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_ritual_waypoints_release.py')))
native, instructions, sites = (previous[key] for key in ('native', 'instructions', 'sites'))


def leaf(prefix, size):
    address = native['symbol'](prefix)
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    body = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + size], address))
    assert sum(entry.size for entry in body) == size
    return body


helper_name = '?CastBlindingLight@instance_silver_hand_InstanceMapScript@'
admission = instructions('?onScenarionNextStep@instance_silver_hand_InstanceMapScript@')
invoke, = sites(admission, helper_name)
comparisons = sites(admission, '??MObjectGuid@@')
assert len(comparisons) == 2
assert admission[comparisons[-1] + 1].op_str == 'al, al'
assert admission[comparisons[-1] + 2].mnemonic == 'je'
assert admission[comparisons[-1] + 2].operands[0].imm == admission[invoke + 2].address
assert all(any(entry.op_str == operands for entry in admission[:invoke]) for operands in (
    'r12, [r15 + 0x2f8]', 'xmm0, xmmword ptr [r15]', 'xmmword ptr [rax + 0x1a], xmm0', 'rcx, r12'))
assert admission[invoke - 3].op_str == 'rdx, r13' and admission[invoke - 1].op_str == 'rcx, r15'
guid_get, = sites(admission, '?GetGuidValue@Object@@')
assert admission[guid_get - 2].op_str == 'edx, edx' and admission[guid_get - 2].mnemonic == 'xor'
guid_order = leaf('??MObjectGuid@@', 28)
assert [entry.op_str for entry in guid_order[:2]] == ['rax, qword ptr [rdx + 8]', 'qword ptr [rcx + 8], rax']
assert [entry.op_str for entry in guid_order[-4:]] == ['rax, qword ptr [rdx]', 'qword ptr [rcx], rax', 'al', '']
factory = instructions('?GetInstanceScript@instance_silver_hand@@')
assert all(any(entry.op_str == operands for entry in factory) for operands in (
    'ecx, 0x308', 'qword ptr [rdi + 0x2f8], rsi', 'qword ptr [rdi + 0x300], rsi', 'qword ptr [rdi + 0x2f8], rax'))
helper = instructions(helper_name)
cast, = sites(helper, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@PEAVItem@@PEBVAuraEffect@@VObjectGuid@@@Z')
flags, = sites(helper, '?SetFlag@Object@@')
arm, = sites(helper, '?AddDelayedEvent@InstanceScript@@')
assert cast < flags < arm
assert [entry.op_str for entry in helper[cast - 3:cast]] == ['r9d, r9d', 'r8d, 0x348dc', 'rcx, rdx']
assert helper[cast - 3].mnemonic == 'xor' and helper[cast + 1].op_str == 'esi, eax'
assert [entry.op_str for entry in helper[flags - 3:flags]] == ['edx, 0x61', 'r8d, 0x2000380', 'rcx, rdi']
assert helper[flags + 1].op_str == 'esi, esi' and helper[flags + 2].mnemonic == 'je'
success_exit, = [entry for entry in helper[arm:] if entry.op_str == 'rcx, qword ptr [rsp + 0x120]']
assert helper[flags + 2].operands[0].imm == success_exit.address
capture, = sites(helper, '?GetGuidValue@Object@@')
assert flags < capture < arm and helper[capture - 2].op_str == 'edx, edx'
assert helper[capture + 1].op_str == 'xmm6, xmmword ptr [rax]'
assert helper[arm - 2].op_str == 'edx, 0x3e8' and helper[arm - 1].op_str == 'rcx, rbx'
assert all(any(entry.op_str == operands for entry in helper[:arm]) for operands in (
    'qword ptr [rsp + 0xc8], rbx', 'xmmword ptr [rsp + 0xd0], xmm6'))
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'CastBlindingLight@instance_silver_hand_InstanceMapScript@' in name]
tables = []
for index, entry in enumerate(helper[:arm]):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            assert helper[index + 1].op_str == 'qword ptr [rsp + 0xc0], rax'
            tables.append(address)
assert len(tables) == 1
thunk = leaf(closure, 9)
lambda_name, = [name for name in native['symbols'] if name.startswith('??R<lambda_1>@') and 'CastBlindingLight@instance_silver_hand_InstanceMapScript@' in name]
assert thunk[0].op_str == 'rcx, 8' and thunk[1].mnemonic == 'jmp' and thunk[1].operands[0].imm == native['symbol'](lambda_name)
callback = instructions(lambda_name)
lookup, = sites(callback, '?GetCreature@Map@@')
retry, = sites(callback, helper_name)
assert [entry.op_str for entry in callback[lookup - 4:lookup]] == ['rsi, [rdi + 8]', 'rcx, qword ptr [rdi]', 'rdx, rsi', 'rcx, qword ptr [rcx + 0x10]']
scenario, = sites(callback, '?GetScenario@ScenarioMgr@@')
assert scenario < lookup < retry
scenario_gate, = [index for index, entry in enumerate(callback) if entry.op_str == 'r14, r14' and callback[index + 1].mnemonic == 'je']
deny = callback[scenario_gate + 1].operands[0].imm
for operands, mnemonic, branch in (('r14, r14', 'test', 'je'), ('eax, 0x444', 'cmp', 'jne'),
                                   ('eax, 0xa', 'cmp', 'jne'), ('rbx, rbx', 'test', 'je'),
                                   ('dword ptr [rbx + 0xa18], 0', 'cmp', 'jne'), ('eax, 0x1a0df', 'cmp', 'jne'),
                                   ('dword ptr [rbx + 0xe0], 0x603', 'cmp', 'jne'), ('qword ptr [rbx + 8], rax', 'cmp', 'jne')):
    index, = [index for index, entry in enumerate(callback[:retry]) if entry.mnemonic == mnemonic and entry.op_str == operands]
    assert callback[index + 1].mnemonic == branch and callback[index + 1].operands[0].imm == deny
for method, branch in (('?IsInWorld@Object@@', 'je'), ('?IsPreDelete@Object@@', 'jne')):
    index, = sites(callback, method)
    assert callback[index + 1].op_str == 'al, al' and callback[index + 2].mnemonic == branch and callback[index + 2].operands[0].imm == deny
assert callback[retry - 1].op_str == 'rdx, rbx'
assert callback[retry + 1].mnemonic == 'jmp' and callback[retry + 1].operands[0].imm > deny
erase_names = [name for name in native['symbols'] if name.startswith('?_Erase@') and 'Tset_traits@VObjectGuid@@' in name]
erase_calls = [(name, index) for name in erase_names for index in sites(callback, name)]
assert len(erase_calls) == 1 and len(sites(callback, '??MObjectGuid@@')) == 3
erase = erase_calls[0][1]
assert callback[erase - 1].op_str == 'rcx, [r12 + 0x2f8]' and callback[erase].address > deny
delay = leaf('?AddDelayedEvent@InstanceScript@@', 12)
assert delay[0].op_str == 'rcx, 0x130' and delay[1].mnemonic == 'jmp' and native['resolve'](delay[1].operands[0].imm) == native['symbol']('?AddDelayedEvent@FunctionProcessor@@')
update = instructions('?UpdateForScript@InstanceScript@@')
drain, = sites(update, '?Update@FunctionProcessor@@')
assert update[drain - 1].op_str == 'rcx, 0x130'
assert len(sites(instructions('?Update@InstanceMap@@'), '?UpdateForScript@InstanceScript@@')) == 1
destroy = instructions('??_Ginstance_silver_hand_InstanceMapScript@')
assert len(sites(destroy, '??1InstanceScript@@')) == 1 and any(entry.op_str == 'edx, 0x308' for entry in destroy)
base_destroy = instructions('??1InstanceScript@@')
release, = sites(base_destroy, '??1FunctionProcessor@@')
assert base_destroy[release - 1].op_str == 'rcx, [rdi + 0x130]'
for body, message in ((helper, b'Silver Hand finale: Blinding Light failed with result %u; retry in1000ms'),
                      (callback, b'Silver Hand finale: dropping stale Blinding Light retry')):
    assert any(entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM
               and entry.operands[1].mem.base == X86_REG_RIP
               and native['binary'].startswith(message, native['offset'](entry.address + entry.size + entry.operands[1].mem.disp)) for entry in body)
    for method in ('?AddDelayedEvent@Unit@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@'):
        assert not sites(body, method)
assert not sites(callback, '?AddDelayedEvent@InstanceScript@@')
print('PASS selected1539 factory0x308/full128-bit GUID set2f8 admits one finale cast before reentry; original nontriggered self215260/protection flags retained. Actual result0 returns before1000ms instance-owned queue130; closure/table/9-byte thunk retains instance+full GUID, resolves Map actor and requires live106719/1539/same map/scenario1092/exact10. Stale work erases the GUID/logs without requeue; success stops retry. Native InstanceMap->UpdateForScript drains same base queue and destruction releases it. No forced casts, Player credit or items; accepted cast is not client aura/VFX/arrival verification.')
