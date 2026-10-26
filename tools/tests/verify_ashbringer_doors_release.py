"""Verify selected Ashbringer instance owns door GUIDs, not retired pointers."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


def leaf(prefix, size):
    address = native['symbol'](prefix)
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    body = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + size], address))
    assert sum(entry.size for entry in body) == size
    return body


scope = 'instance_broken_shore_InstanceMapScript@'
methods = {name: '?' + name + '@' + scope for name in ('Initialize', 'OnGameObjectCreate', 'OnGameObjectRemove', 'onScenarionNextStep', 'CreatureDies')}
factory = instructions('?GetInstanceScript@instance_broken_shore@@')
tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<64Q', native['binary'], native['offset'](address))
        if all(slots[index // 8] == native['symbol'](methods[name]) for name, index in
               (('Initialize', 0xc8), ('OnGameObjectCreate', 0x30), ('OnGameObjectRemove', 0x38), ('onScenarionNextStep', 0x118), ('CreatureDies', 0x28))):
            tables.append(address)
assert len(tables) == 1
assert all(any(entry.op_str == value for entry in factory) for value in
           ('ecx, 0x318', 'qword ptr [rdi + 0x2f0], rsi', 'qword ptr [rdi + 0x2f8], rsi', 'qword ptr [rdi + 0x2f0], rax'))
create = instructions(methods['OnGameObjectCreate'])
getter, = sites(create, '?GetGuidValue@Object@@')
assert create[getter - 2].op_str == 'edx, edx' and create[getter - 2].mnemonic == 'xor'
assert any(entry.op_str == 'r14, [rdi + 0x2f0]' for entry in create[:getter])
assert len(sites(create, '??MObjectGuid@@')) == 2
assert any(entry.op_str == 'xmm0, xmmword ptr [rbp]' for entry in create) and any(entry.op_str == 'xmmword ptr [rax + 0x1a], xmm0' for entry in create)
assert any(entry.op_str == 'eax, 0x3b803' for entry in create) and any(entry.op_str == 'eax, 0x65' for entry in create) and any(entry.op_str == 'eax, 0x862' for entry in create)
remove = instructions(methods['OnGameObjectRemove'])
getter, = sites(remove, '?GetGuidValue@Object@@')
assert remove[getter - 1].op_str == 'edx, edx' and len(sites(remove, '??MObjectGuid@@')) == 3
assert any(entry.op_str == 'rcx, [r13 + 0x2f0]' for entry in remove)
assert remove[-1].mnemonic == 'jmp' and native['resolve'](remove[-1].operands[0].imm) == native['symbol']('?OnGameObjectRemove@InstanceScript@@')
assert any(entry.op_str == 'rdx, r12' for entry in remove) and any(entry.op_str == 'rcx, r13' for entry in remove[-12:])
copy_name, = [name for name in native['symbols'] if name.startswith('??0?$set@VObjectGuid@@') and name.endswith('AEBV01@@Z')]
copy = instructions(copy_name)
copy_nodes, = [name for name in native['symbols'] if name.startswith('??$_Copy_nodes@') and 'Tset_traits@VObjectGuid@@' in name]
assert len(sites(copy, copy_nodes)) == 1
nodes = instructions(copy_nodes)
assert any(entry.op_str == 'xmm0, xmmword ptr [rsi + 0x1a]' for entry in nodes) and any(entry.op_str == 'xmmword ptr [rax + 0x1a], xmm0' for entry in nodes)
stage, death = (instructions(methods[name]) for name in ('onScenarionNextStep', 'CreatureDies'))
assert stage[0].mnemonic == 'cmp' and stage[0].op_str == 'edx, 2' and stage[1].mnemonic == 'jne'
for body, stack, equality, branch, identity in ((stage, '0x20', '??8ObjectGuid@@', 'je', '0x3b803'),
                                                (death, '0x30', '??9ObjectGuid@@', 'jne', '0x3b868')):
    copied, = sites(body, copy_name)
    lookup, = sites(body, '?GetGameObject@Map@@')
    check, = sites(body, equality)
    handle, = sites(body, '?HandleGameObject@InstanceScript@@')
    assert copied < lookup < check < handle
    assert body[copied - 1].op_str == f'rcx, [rsp + {stack}]'
    assert body[copied - 2].op_str in ('rdx, [rcx + 0x2f0]', 'rdx, [rbp + 0x2f0]')
    assert body[lookup - 2].op_str == 'rdx, [rbx + 0x1a]' and body[lookup - 1].op_str == 'rcx, qword ptr [rbp + 0x10]'
    assert body[lookup + 2].op_str == 'rax, rax' and body[lookup + 3].mnemonic == 'je'
    skip = body[lookup + 3].operands[0].imm
    assert body[check - 2].op_str == 'rdx, [rbx + 0x1a]' and body[check - 1].op_str == 'rcx, rax'
    assert body[check + 1].op_str == 'al, al' and body[check + 2].mnemonic == branch and body[check + 2].operands[0].imm == skip
    assert any(entry.mnemonic == 'cmp' and entry.op_str == f'eax, {identity}' for entry in body[check:handle])
    assert any(entry.op_str == 'xmm0, xmmword ptr [rbx + 0x1a]' for entry in body[check:handle])
    assert body[handle - 4].op_str == 'r9d, r9d' and body[handle - 3].op_str == 'r8b, 1'
    assert any(entry.mnemonic == 'je' and entry.operands[0].imm == body[lookup - 2].address for entry in body[handle:])
for prefix, success, failure in (('??8ObjectGuid@@', ('mov', 'al, 1'), ('xor', 'al, al')),
                                  ('??9ObjectGuid@@', ('xor', 'al, al'), ('mov', 'al, 1'))):
    comparison = leaf(prefix, 24)
    assert [entry.op_str for entry in comparison[:6:3]] == ['rax, qword ptr [rdx]', 'rax, qword ptr [rdx + 8]']
    assert [entry.op_str for entry in comparison[1:6:3]] == ['qword ptr [rcx], rax', 'qword ptr [rcx + 8], rax']
    assert (comparison[6].mnemonic, comparison[6].op_str) == success and (comparison[8].mnemonic, comparison[8].op_str) == failure
assert any(entry.op_str == 'eax, 0x3c0ca' for entry in death)
phase, = [index for index, entry in enumerate(death) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0xa0]']
assert [entry.op_str for entry in death[phase - 3:phase]] == ['r8b, 1', 'edx, 1', 'rcx, rdi']
assert len(sites(death, '?DoUpdateAchievementCriteria@InstanceScript@@')) == 2
assert {52858, 52845, 52846, 52847, 46064} <= {operand.imm for entry in death for operand in entry.operands if operand.type == CS_OP_IMM}
for method, slot in (('?AddToWorld@GameObject@@', 0x30), ('?RemoveFromWorld@GameObject@@', 0x38)):
    assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == slot for entry in instructions(method))
kill = instructions('?Kill@Unit@@')
dispatch, = [index for index, entry in enumerate(kill) if entry.op_str == 'r9, qword ptr [rcx + 0x28]']
assert [entry.op_str for entry in kill[dispatch:dispatch + 5]] == ['r9, qword ptr [rcx + 0x28]', 'r8, r13', 'rdx, r15', 'rcx, rax', 'r9']
assert kill[dispatch + 4].mnemonic == 'call'
for body in (create, remove, stage, death):
    for method in ('?AddItem@Player@@', '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@PEAVItem@@PEBVAuraEffect@@VObjectGuid@@@Z'):
        assert not sites(body, method)
print('PASS selected1500 factory318 binds Initialize/c8, create30, remove38, death28, step118. Actual member2f0 stores full128-bit GUIDs; removal erases and tail-calls native base bookkeeping. Both door loops make native independent set snapshots/copy16-byte nodes, resolve actual map GUIDs, skip missing and explicitly compare both64-bit GUID halves before original243715/243816 open or245962 phase1/update. Native create/remove/death dispatch and original five92 criteria retained; no item/spell bypass. Actual door rendering/collision, late-create replay and whole Ashbringer/client traversal remain separate.')
