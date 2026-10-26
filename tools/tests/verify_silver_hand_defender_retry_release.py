"""Verify empty-defender and zero-creation recovery in the selected native AI."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_ritual_progress_release.py')))
native, instructions, sites = previous['native'], previous['instructions'], previous['sites']
lifetime = previous['previous']['lifetime']
retry_name = '?RetryWave@npc_travar_lastAI@'
summoning = instructions('?SummonAdds@npc_travar_lastAI@')
retry_calls = sites(summoning, retry_name)
assert len(retry_calls) == 3
retry_by_actor = {}
for retry_call in retry_calls:
    reason = summoning[retry_call - 3]
    assert reason.mnemonic == 'mov' and reason.op_str.startswith('r8d, ')
    assert [entry.op_str for entry in summoning[retry_call - 2:retry_call]] == ['edx, bl', 'rcx, rdi']
    retry_by_actor[reason.operands[1].imm] = reason.address
assert set(retry_by_actor) == {106774, 106580, 106719}
retry_entry = retry_by_actor[106774]
lookups = sites(summoning, '?FindNearestCreature@WorldObject@@')
assert len(lookups) == 2
for lookup in lookups:
    assert summoning[lookup - 1].op_str == 'edx, 0x1a116'
    assert summoning[lookup - 2].op_str == 'rcx, qword ptr [rdi + 0x20]'
    radius = summoning[lookup - 4]
    assert radius.mnemonic == 'movss' and radius.op_str.startswith('xmm2, dword ptr [rip')
    assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 70.0
    assert summoning[lookup + 2].op_str == 'rax, rax'
    assert summoning[lookup + 3].mnemonic == 'je' and summoning[lookup + 3].operands[0].imm == retry_entry
incoming = [index for index, entry in enumerate(summoning) if entry.mnemonic.startswith('j')
            and entry.operands[0].type == CS_OP_IMM and entry.operands[0].imm == retry_entry]
assert incoming == [lookup + 3 for lookup in lookups]
selector = next(index for index, entry in enumerate(summoning) if entry.op_str == 'ebx, dl')
assert [entry.op_str for entry in summoning[selector:selector + 3]] == ['ebx, dl', 'ecx, ebx', 'ecx, 1']
assert summoning[selector + 2].mnemonic == 'sub' and summoning[selector + 4].mnemonic == 'sub'
assert summoning[selector + 4].op_str == 'ecx, 1' and summoning[selector + 6].op_str == 'ecx, 1'
assert summoning[selector + 6].mnemonic == 'cmp'
for index in (selector + 3, selector + 5):
    assert summoning[index].mnemonic == 'je' and summoning[index].operands[0].imm == summoning[lookups[1] - 4].address
assert summoning[selector + 7].mnemonic == 'jne' and summoning[selector + 7].operands[0].imm < retry_entry
assert summoning[lookups[0] - 3].op_str == 'r9d, cl' and summoning[lookups[1] - 3].op_str == 'r9b, 1'
creations = sites(summoning, '?SummonCreature@WorldObject@@QEAAPEAVTempSummon@@IMMMM')
assert len(creations) == 3
for creation, actor in zip(creations, (106719, 106580, 106578)):
    assert any(entry.mnemonic == 'mov' and entry.op_str == f'edx, {hex(actor)}' for entry in summoning[creation - 16:creation])
    assert summoning[creation + 2].op_str == 'rax, rax'
    branch = summoning[creation + 3]
    assert branch.mnemonic == 'je'
    destination = branch.operands[0].imm
    if actor != 106578:
        assert destination == retry_by_actor[actor]
        incoming_creation = [entry for entry in summoning if entry.mnemonic.startswith('j')
                             and entry.operands[0].type == CS_OP_IMM and entry.operands[0].imm == destination]
        assert incoming_creation == [branch]
    else:
        target = next(index for index, entry in enumerate(summoning) if entry.address == destination)
        assert summoning[target].mnemonic == 'sub' and summoning[target].op_str == 'rsi, 1'
        assert summoning[target + 1].mnemonic == 'jne' and summoning[target + 1].operands[0].imm < summoning[creation].address

retry = instructions(retry_name)
arm, = sites(retry, '?AddDelayedEvent@FunctionProcessor@@')
assert retry[arm - 1].op_str == 'edx, 0x3e8'
assert any(entry.op_str == 'rcx, [rbx + 0x48]' for entry in retry[:arm])
assert any(entry.op_str == 'edi, dl' for entry in retry[:arm]) and any(entry.op_str == 'rbx, rcx' for entry in retry[:arm])
assert any(entry.op_str == 'qword ptr [rsp + 0x98], rbx' for entry in retry[:arm])
assert any(entry.op_str == 'byte ptr [rsp + 0xa0], dil' for entry in retry[:arm])
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'RetryWave@npc_travar_lastAI@' in name]
tables = []
for index, entry in enumerate(retry[:arm]):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            assert retry[index + 1].op_str == 'qword ptr [rsp + 0x90], rax'
            tables.append(address)
assert len(tables) == 1
thunk = lifetime['leaf'](closure, 9)
lambda_name, = [name for name in native['symbols'] if name.startswith('??R<lambda_1>@') and 'RetryWave@npc_travar_lastAI@' in name]
assert thunk[0].mnemonic == 'add' and thunk[0].op_str == 'rcx, 8'
assert thunk[1].mnemonic == 'jmp' and thunk[1].operands[0].imm == native['symbol'](lambda_name)
callback = instructions(lambda_name)
gate, = sites(callback, lifetime['wave_helper'])
spawn, = sites(callback, '?SummonAdds@npc_travar_lastAI@')
assert [entry.op_str for entry in callback[gate - 3:gate]] == ['rbx, rcx', 'rcx, qword ptr [rcx]', 'rcx, qword ptr [rcx + 0x20]']
assert callback[gate + 1].op_str == 'al, al' and callback[gate + 2].mnemonic == 'je'
assert callback[gate + 2].operands[0].imm == callback[spawn + 2].address
assert [entry.op_str for entry in callback[spawn - 2:spawn]] == ['edx, byte ptr [rbx + 8]', 'rcx, qword ptr [rbx]']
assert callback[spawn + 1].mnemonic == 'jmp' and callback[spawn + 1].operands[0].imm > callback[spawn + 2].address
for body, message in ((retry, b'Silver Hand ritual: actor %u unavailable for wave %u; retry in1000ms'),
                      (callback, b'Silver Hand ritual: dropping retry for stale wave %u')):
    assert any(entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM
               and entry.operands[1].mem.base == X86_REG_RIP
               and native['binary'].startswith(message, native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))
               for entry in body)
    for method in ('?AddDelayedEvent@Unit@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?FindNearestPlayer@WorldObject@@', '?AddItem@Player@@'):
        assert not sites(body, method)
assert not sites(callback, '?AddDelayedEvent@FunctionProcessor@@')
print('PASS selected wave1/2/3 switch: two empty live106774/70yd lookups plus null106580 leader/null106719 creation route to actual RetryWave with exact reason IDs and original AI/wave byte; null106578 add only continues existing two-add loop, never replays a populated wave. Native function-object table/9-byte thunk captures AI+wave, queues1000ms at AI+48 and reuses verified live1092/exact9 policy before fresh SummonAdds; stale retry logs/drops without direct credit or self-requeue. Factory/locked lifecycle/destruction/owned-death/progress retained. Custom recovery cadence, partial creation, attempt epochs and client combat/VFX remain separate.')
