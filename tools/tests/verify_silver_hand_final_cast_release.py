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
insert_name, = [name for name in native['symbols'] if name.startswith('??$insert@') and 'Tset_traits@VObjectGuid@@' in name and name.endswith('$$QEAVObjectGuid@@@Z')]
insert_sites = sites(admission, insert_name)
assert len(insert_sites) == 2
guid_gets = sites(admission, '?GetGuidValue@Object@@')
assert len(guid_gets) == 2
talk, = sites(admission, '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')
for getter, insert, effect, skip in ((guid_gets[0], insert_sites[0], talk, talk + 1), (guid_gets[1], insert_sites[1], invoke, invoke + 1)):
    assert getter < insert < effect
    assert admission[getter - 3].op_str == 'edx, edx' and admission[getter - 3].mnemonic == 'xor'
    assert admission[getter + 1].op_str == 'rcx, [r15 + 0x2f8]' and admission[getter + 2].op_str == 'r8, rax'
    assert admission[insert + 1].op_str == 'byte ptr [rax + 8], sil' and admission[insert + 2].mnemonic == 'je'
    assert admission[insert + 2].operands[0].imm == admission[skip].address
insert_body = instructions(insert_name)
comparisons = sites(insert_body, '??MObjectGuid@@')
assert len(comparisons) == 2
assert insert_body[comparisons[-1] + 1].op_str == 'al, al' and insert_body[comparisons[-1] + 2].mnemonic == 'je'
duplicate = next(index for index, entry in enumerate(insert_body) if entry.address == insert_body[comparisons[-1] + 2].operands[0].imm)
assert insert_body[duplicate].op_str == 'qword ptr [r14], rdi' and insert_body[duplicate + 1].op_str == 'byte ptr [r14 + 8], al'
assert all(any(entry.op_str == operands for entry in insert_body[:duplicate]) for operands in ('rbp, r8', 'xmm0, xmmword ptr [rbp]', 'xmmword ptr [rax + 0x1a], xmm0', 'al, 1'))
assert admission[invoke - 2].op_str == 'rdx, rdi' and admission[invoke - 1].op_str == 'rcx, r15'
guid_order = leaf('??MObjectGuid@@', 28)
assert [entry.op_str for entry in guid_order[:2]] == ['rax, qword ptr [rdx + 8]', 'qword ptr [rcx + 8], rax']
assert [entry.op_str for entry in guid_order[-4:]] == ['rax, qword ptr [rdx]', 'qword ptr [rcx], rax', 'al', '']
factory = instructions('?GetInstanceScript@instance_silver_hand@@')
assert all(any(entry.op_str == operands for entry in factory) for operands in (
    'ecx, 0x310', 'qword ptr [rdi + 0x2f8], rsi', 'qword ptr [rdi + 0x300], rsi', 'qword ptr [rdi + 0x2f8], rax', 'byte ptr [rdi + 0x308], sil'))
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
identity, = sites(callback, '??9ObjectGuid@@')
guid_get, = sites(callback, '?GetGuidValue@Object@@')
assert lookup < guid_get < identity < retry
assert callback[guid_get - 2].op_str == 'edx, edx' and callback[guid_get - 1].op_str == 'rcx, rbx'
assert [entry.op_str for entry in callback[identity - 2:identity]] == ['rdx, rsi', 'rcx, rax']
assert callback[identity + 1].op_str == 'al, al' and callback[identity + 2].mnemonic == 'jne'
assert callback[identity + 2].operands[0].imm == deny
inequality = leaf('??9ObjectGuid@@', 24)
assert [entry.op_str for entry in inequality[:6:3]] == ['rax, qword ptr [rdx]', 'rax, qword ptr [rdx + 8]']
assert [entry.op_str for entry in inequality[1:6:3]] == ['qword ptr [rcx], rax', 'qword ptr [rcx + 8], rax']
assert all(inequality[index].mnemonic == 'jne' and inequality[index].operands[0].imm == inequality[8].address for index in (2, 5))
assert inequality[6].mnemonic == 'xor' and inequality[6].op_str == 'al, al'
assert inequality[8].mnemonic == 'mov' and inequality[8].op_str == 'al, 1'
assert inequality[7].mnemonic == 'ret' and inequality[9].mnemonic == 'ret'
for operands, mnemonic, branch in (('r14, r14', 'test', 'je'), ('eax, 0x444', 'cmp', 'jne'),
                                   ('eax, 0xa', 'cmp', 'jne'), ('rbx, rbx', 'test', 'je'),
                                   ('dword ptr [rbx + 0xa18], 0', 'cmp', 'jne'), ('eax, 0x1a0df', 'cmp', 'jne'),
                                   ('dword ptr [rbx + 0xe0], 0x603', 'cmp', 'jne'), ('qword ptr [rbx + 8], rax', 'cmp', 'jne')):
    index, = [index for index, entry in enumerate(callback[:retry]) if entry.mnemonic == mnemonic and entry.op_str == operands]
    assert callback[index + 1].mnemonic == branch and callback[index + 1].operands[0].imm == deny
for method, branch in (('?IsInWorld@Object@@', 'je'), ('?IsPreDelete@Object@@', 'jne')):
    index, = sites(callback, method)
    assert identity < index
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
assert len(sites(destroy, '??1InstanceScript@@')) == 1 and any(entry.op_str == 'edx, 0x310' for entry in destroy)
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
print('PASS selected1539 factory0x310/full128-bit GUID set2f8 admits both finale speech/cast before reentry through native insert/128-bit comparator/copy; original nontriggered self215260/protection flags retained. Result0 returns before1000ms instance queue130; closure/table/thunk retains instance+full GUID. After native Map lookup, explicit native24-byte inequality compares both GUID halves against captured identity before live106719/1539/same-map/1092/exact10 checks and retry. Same-low alias follows unchanged stale erase/log/drop without replacement cast/flags or self-requeue. Native queue dispatch/destruction retained; no forced casts, Player credit or items. Accepted cast is not client aura/VFX/arrival verification.')
