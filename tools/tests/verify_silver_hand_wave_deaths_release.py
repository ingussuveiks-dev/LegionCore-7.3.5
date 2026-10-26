"""Verify last Travard's selected full-GUID summon roster and death thresholds."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_final_entry_release.py')))
native, instructions, sites = previous['native'], previous['instructions'], previous['sites']
lifetime = previous['previous']
slots, factory = lifetime['slots'], lifetime['factory']
registration_name = '?JustSummoned@npc_travar_lastAI@'
death_name = '?SummonedCreatureDies@npc_travar_lastAI@'
assert slots[0x140 // 8] == native['symbol'](registration_name)
assert slots[0x158 // 8] == native['symbol'](death_name)
assert all(any(entry.op_str == operands for entry in factory) for operands in
           ('ecx, 0xd8', 'qword ptr [rdi + 0xc8], rax', 'qword ptr [rdi + 0xd0], rax',
            'word ptr [rax + 0x18], 0x101'))
zero = next(index for index, entry in enumerate(factory) if entry.op_str == 'eax, eax' and entry.mnemonic == 'xor')
assert [entry.op_str for entry in factory[zero + 1:zero + 3]] == [
    'qword ptr [rdi + 0xc8], rax', 'qword ptr [rdi + 0xd0], rax']


def position(body, operands, mnemonic=None):
    found = [index for index, entry in enumerate(body) if entry.op_str == operands
             and (mnemonic is None or entry.mnemonic == mnemonic)]
    result, = found
    return result


def guarded(body, operands, mnemonic, branch, target):
    index = position(body, operands, mnemonic)
    assert body[index + 1].mnemonic == branch and body[index + 1].operands[0].imm == target
    return index


def named_calls(body, prefix):
    targets = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith(prefix)}
    assert targets
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) in targets]


registration = instructions(registration_name)
deny = registration[position(registration, 'rbx, qword ptr [rsp + 0x70]', 'mov')].address
for operands, mnemonic, branch in (
        ('byte ptr [rcx + 0x38], 0', 'cmp', 'je'), ('rdx, rdx', 'test', 'je'),
        ('dword ptr [rdx + 0xa18], 0', 'cmp', 'jne'), ('eax, 0x1a052', 'cmp', 'jne'),
        ('rax, rax', 'test', 'je'), ('qword ptr [rbx + 8], rax', 'cmp', 'jne'),
        ('dword ptr [rbx + 0x3b8], eax', 'cmp', 'jne')):
    if operands == 'rax, rax':
        for index, entry in enumerate(registration):
            if entry.mnemonic == mnemonic and entry.op_str == operands:
                assert registration[index + 1].mnemonic == branch and registration[index + 1].operands[0].imm == deny
    else:
        guarded(registration, operands, mnemonic, branch, deny)
entry_gate = position(registration, 'eax, 0x1a054', 'cmp')
assert registration[entry_gate + 1].mnemonic == 'je'
temporary_gate = position(registration, 'byte ptr [rbx + 0x30e8], 1', 'test')
assert registration[entry_gate + 1].operands[0].imm == registration[temporary_gate].address
assert [entry.op_str for entry in registration[temporary_gate + 1:temporary_gate + 5]] == [
    'rax, rbx', 'r15d, 0', 'rax, r15', 'rax, rax']
assert registration[temporary_gate + 3].mnemonic == 'cmove'
owner, = sites(registration, '??9ObjectGuid@@')
assert any(entry.op_str == 'xmm0, xmmword ptr [rax + 0x3b64]' for entry in registration[:owner])
assert registration[owner + 1].op_str == 'al, al'
assert registration[owner + 2].mnemonic == 'jne' and registration[owner + 2].operands[0].imm == deny
assert len(sites(registration, '??MObjectGuid@@')) == 2
roster = position(registration, 'r14, [rdi + 0xc8]', 'lea')
copy_guid = position(registration, 'xmm0, xmmword ptr [rbp]', 'movups')
copy_node = position(registration, 'xmmword ptr [rax + 0x1a], xmm0', 'movups')
insert, = named_calls(registration, '?_Insert_node@')
assert owner < roster < copy_guid < copy_node < insert
assert registration[insert - 1].op_str == 'rcx, r14'

death = instructions(death_name)
assert [(entry.mnemonic, entry.op_str) for entry in death[-4:]] == [
    ('add', 'rsp, 0x38'), ('pop', 'rsi'), ('pop', 'rbx'), ('ret', '')]
deny = death[-4].address
guarded(death, 'rdx, rdx', 'test', 'je', death[-1].address)
assert death[-1].mnemonic == 'ret'
dead = position(death, 'dword ptr [rdx + 0xa18], 0', 'cmp')
assert death[dead + 3].mnemonic == 'je' and death[dead + 3].operands[0].imm == deny
for method, branch in (('?IsInWorld@Object@@', 'je'), ('?IsPreDelete@Object@@', 'jne')):
    call, = sites(death, method)
    assert death[call + 1].op_str == 'al, al'
    assert death[call + 2].mnemonic == branch and death[call + 2].operands[0].imm == deny
guarded(death, 'dword ptr [rax + 0xa18], 0', 'cmp', 'jne', deny)
cap = guarded(death, 'byte ptr [rsi + 0x40], 6', 'cmp', 'jae', deny)
assert len(sites(death, '??MObjectGuid@@')) == 3
assert any(entry.op_str == 'r15, qword ptr [rsi + 0xc8]' for entry in death)
erase, = named_calls(death, '?_Erase@')
assert death[erase - 2].op_str == 'rcx, [rsi + 0xc8]'
erased = guarded(death, 'rax, rax', 'test', 'je', deny)
increment = position(death, 'al', 'inc')
assert death[increment - 1].op_str == 'eax, byte ptr [rsi + 0x40]'
assert death[increment + 1].op_str == 'byte ptr [rsi + 0x40], al'
assert cap < erase < erased < increment
threshold_three = position(death, 'al, 3', 'cmp')
threshold_six = guarded(death, 'al, 6', 'cmp', 'jne', deny)
wave_two = position(death, 'dl, 2', 'mov')
wave_three = position(death, 'dl, 3', 'mov')
assert death[threshold_three + 1].mnemonic == 'je'
assert death[threshold_three + 1].operands[0].imm == death[wave_two].address
waves = sites(death, '?SummonAdds@npc_travar_lastAI@')
assert len(waves) == 2 and increment < threshold_three < threshold_six < wave_three < waves[0] < wave_two < waves[1]
for method in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@'):
    assert not sites(registration, method) and not sites(death, method)

initialization = instructions('?InitSummon@TempSummon@@')
dispatch = [index for index, entry in enumerate(initialization) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp in (0x140, 0x148)]
assert len(dispatch) == 2 and [initialization[index].operands[0].mem.disp for index in dispatch] == [0x140, 0x148]
assert initialization[dispatch[0] - 3].op_str == 'rcx, qword ptr [rax + 0x9c0]'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM
           and entry.operands[0].mem.disp == 0x158 for entry in instructions('?Kill@Unit@@'))
summoning = instructions('?SummonAdds@npc_travar_lastAI@')
fallbacks = [index for index, entry in enumerate(summoning) if entry.mnemonic == 'mov' and entry.op_str == 'r8, qword ptr [rcx + 0x140]']
assert len(fallbacks) == 2
for index in fallbacks:
    assert any(entry.op_str == 'rdx, rax' and entry.mnemonic == 'mov' for entry in summoning[index - 5:index])
    assert summoning[index + 1].op_str == 'rcx, rdi'
    following = [entry for entry in summoning[index + 2:index + 5] if entry.mnemonic != 'movaps']
    assert following[0].mnemonic == 'call' and following[0].op_str == 'r8'
destroy = lifetime['destroy']
release = lifetime['release']
assert named_calls(destroy[:release], '??$_Erase_tree@')
assert any(entry.op_str == 'rcx, [rdi + 0xc8]' for entry in destroy[:release])
assert any(entry.op_str == 'edx, 0xd8' for entry in destroy[release:])
print('PASS: canonical last Travard factory selects roster registration0x140 and native Kill death0x158; full128-bit owned/live regular summons only, set erasure before bounded3/6 increment, duplicate/null/alive/untracked rejection, no Player credit. Native InitSummon registers before IsSummonedBy, both successful regular return paths register idempotently, and AI destruction releases roster plus original queue. This does not certify disabled-AI death redelivery, failed-spawn recovery, periodic51838 timing or client combat/VFX.')
