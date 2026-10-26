"""Verify recovery of individual uncreated106578 slots, never their parent wave."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_defender_retry_release.py')))
native, instructions, sites, lifetime = (previous[key] for key in ('native', 'instructions', 'sites', 'lifetime'))
helper_name = '?SummonWaveAdd@npc_travar_lastAI@'
summoning = previous['summoning']
slot_call, = sites(summoning, helper_name)
assert summoning[slot_call + 1].mnemonic == 'sub' and summoning[slot_call + 1].op_str == 'rdi, 1'
assert summoning[slot_call + 2].mnemonic == 'jne' and summoning[slot_call + 2].operands[0].imm < summoning[slot_call].address
assert any(entry.mnemonic == 'mov' and entry.op_str == 'edi, 2' for entry in summoning[:slot_call])
assert all(any(entry.op_str == operands for entry in summoning[slot_call - 10:slot_call]) for operands in (
    'qword ptr [rsp + 0x28], rbp', 'dword ptr [rsp + 0x20], xmm8', 'xmm3, xmm9', 'xmm2, xmm7', 'rcx, rbx', 'xmm1, xmm6'))
helper = instructions(helper_name)
creation, = sites(helper, '?SummonCreature@WorldObject@@QEAAPEAVTempSummon@@IMMMM')
arm, = sites(helper, '?AddDelayedEvent@FunctionProcessor@@')
defender_gate, = [index for index, entry in enumerate(helper) if entry.mnemonic == 'test' and entry.op_str == 'r14, r14']
assert helper[defender_gate - 2].op_str == 'r14, qword ptr [rsp + 0x1e8]'
assert helper[defender_gate + 1].mnemonic == 'je'
assert helper[creation + 2].op_str == 'rax, rax' and helper[creation + 3].mnemonic == 'je'
failure = helper[creation + 3].operands[0].imm
assert helper[defender_gate + 1].operands[0].imm == failure
assert all(any(entry.op_str == operands for entry in helper[:creation]) for operands in (
    'xmm6, xmm1', 'xmm7, xmm2', 'xmm8, xmm3', 'xmm9, dword ptr [rsp + 0x1e0]',
    'dword ptr [rsp + 0x28], xmm9', 'dword ptr [rsp + 0x20], xmm3', 'xmm3, xmm2', 'xmm2, xmm1',
    'edx, 0x1a052', 'rcx, qword ptr [rcx + 0x20]'))
attack, = [index for index, entry in enumerate(helper[creation:arm], creation) if entry.mnemonic == 'call'
           and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x10]
assert helper[attack - 1].op_str == 'rdx, r14'
assert helper[attack + 1].mnemonic == 'jmp' and helper[attack + 1].operands[0].imm > helper[arm].address
assert helper[arm - 1].op_str == 'edx, 0x3e8'
assert all(any(entry.op_str == operands for entry in helper[:arm]) for operands in (
    'rcx, [rdi + 0x48]', 'qword ptr [rsp + 0xe8], r15', 'r15, rcx',
    'dword ptr [rsp + 0xf0], xmm6', 'dword ptr [rsp + 0xf4], xmm7',
    'dword ptr [rsp + 0xf8], xmm8', 'dword ptr [rsp + 0xfc], xmm9'))
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'SummonWaveAdd@npc_travar_lastAI@' in name]
tables = []
for index, entry in enumerate(helper[:arm]):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            assert helper[index + 1].op_str == 'qword ptr [rsp + 0xe0], rax'
            tables.append(address)
assert len(tables) == 1
thunk = lifetime['leaf'](closure, 9)
lambda_name, = [name for name in native['symbols'] if name.startswith('??R<lambda_1>@') and 'SummonWaveAdd@npc_travar_lastAI@' in name]
assert thunk[0].mnemonic == 'add' and thunk[0].op_str == 'rcx, 8'
assert thunk[1].mnemonic == 'jmp' and thunk[1].operands[0].imm == native['symbol'](lambda_name)
callback = instructions(lambda_name)
gate, = sites(callback, lifetime['wave_helper'])
lookup, = sites(callback, '?FindNearestCreature@WorldObject@@')
recover, = sites(callback, helper_name)
assert gate < lookup < recover
assert [entry.op_str for entry in callback[gate - 3:gate]] == ['rdi, rcx', 'rcx, qword ptr [rcx]', 'rcx, qword ptr [rcx + 0x20]']
assert callback[gate + 1].op_str == 'al, al' and callback[gate + 2].mnemonic == 'je'
assert callback[gate + 2].operands[0].imm == callback[recover + 2].address
assert all(any(entry.op_str == operands for entry in callback[gate:lookup]) for operands in (
    'rbx, qword ptr [rdi]', 'r9b, 1', 'edx, 0x1a116', 'rcx, qword ptr [rbx + 0x20]'))
radius, = [entry for entry in callback[gate:lookup] if entry.mnemonic == 'movss' and entry.op_str.startswith('xmm2, dword ptr [rip')]
assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 70.0
assert [entry.op_str for entry in callback[lookup + 1:recover]] == [
    'qword ptr [rsp + 0x28], rax', 'xmm0, dword ptr [rdi + 0x14]', 'dword ptr [rsp + 0x20], xmm0',
    'xmm3, dword ptr [rdi + 0x10]', 'xmm2, dword ptr [rdi + 0xc]', 'xmm1, dword ptr [rdi + 8]', 'rcx, rbx']
assert callback[recover + 1].mnemonic == 'jmp' and callback[recover + 1].operands[0].imm > callback[recover + 2].address
for body, message in ((helper, b'Silver Hand ritual: add106578 slot unavailable at %.2f %.2f %.2f; retry in1000ms'),
                      (callback, b'Silver Hand ritual: dropping stale add106578 slot retry')):
    assert any(entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM
               and entry.operands[1].mem.base == X86_REG_RIP
               and native['binary'].startswith(message, native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))
               for entry in body)
    for method in ('?SummonAdds@npc_travar_lastAI@', '?RetryWave@npc_travar_lastAI@', '?AddDelayedEvent@Unit@@',
                   '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@'):
        assert not sites(body, method)
assert not sites(callback, '?AddDelayedEvent@FunctionProcessor@@')
print('PASS selected original two-slot loop calls one106578 helper; null defender/null creation alone enter one1000ms AI+48 job, successful native owned registration/AttackStart skips retry. Actual closure table/9-byte thunk retains only AI+four float pose values, not leader/defender/Player; native9 gate precedes fresh live106774/70yd lookup and exact captured-pose106578 helper. Stale callback logs/drops; neither helper nor callback can replay SummonAdds/RetryWave, credit Players or grant items. Original death3/6/roster/factory/lifecycle proofs retained. Controlled retry cadence, quest epochs and client/VFX remain separate.')
