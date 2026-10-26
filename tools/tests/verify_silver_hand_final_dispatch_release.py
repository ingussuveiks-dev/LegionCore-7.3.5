"""Verify selected step10 independently resolves and handles each final actor."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_ritual_dispatch_release.py')))
native, body = previous['native'], previous['body']
last, boss = previous['lookups'][1:]


def sites(prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


stage = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'edx, 0xa')
assert stage < last < boss
radius, = [entry for entry in body[stage:last] if entry.mnemonic == 'movss' and entry.op_str.startswith('xmm6, dword ptr [rip')]
assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 70.0
for register in ('r12b', 'r14b'):
    assert any(entry.mnemonic == 'xor' and entry.op_str == f'{register}, {register}' for entry in body[stage:last])
assert [entry.op_str for entry in body[last - 4:last]] == ['r9b, 1', 'xmm2, xmm6', 'edx, 0x1a119', 'rcx, rdi']
assert [entry.op_str for entry in body[boss - 4:boss]] == ['r9b, 1', 'xmm2, xmm6', 'edx, 0x1a0df', 'rcx, rdi']
talk, = sites('?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')
cast, = sites('?CastBlindingLight@instance_silver_hand_InstanceMapScript@')
assert last < talk < boss < cast
last_gate = next(index for index in range(stage, last) if body[index].mnemonic == 'test' and body[index].op_str == 'r12b, r12b')
boss_gate = next(index for index in range(talk, boss) if body[index].mnemonic == 'test' and body[index].op_str == 'r14b, r14b')
assert body[last_gate + 1].mnemonic == 'jne' and body[last_gate + 1].operands[0].imm == body[boss_gate].address
assert body[last + 2].op_str == 'rax, rax' and body[last + 3].mnemonic == 'je'
assert body[last + 3].operands[0].imm == body[boss_gate].address
assert any(entry.op_str == 'dl, 3' for entry in body[last:talk])
assert any(entry.mnemonic == 'mov' and entry.op_str == 'r12b, 1' for entry in body[last:talk])
assert any(entry.mnemonic == 'mov' and entry.op_str == 'r14b, 1' for entry in body[boss:cast])
assert body[boss + 2].op_str == 'rax, rax' and body[boss + 3].mnemonic == 'je'
assert body[boss_gate + 1].mnemonic == 'jne'
completion_address = body[boss_gate + 1].operands[0].imm
completion = next(index for index, entry in enumerate(body) if entry.address == completion_address)
assert cast < completion
assert [(entry.mnemonic, entry.op_str) for entry in body[completion:completion + 4:2]] == [('test', 'r12b, r12b'), ('test', 'r14b, r14b')]
assert body[completion + 1].mnemonic == 'je' and body[completion + 3].mnemonic == 'jne'
advance_address = body[completion + 1].operands[0].imm
advance = next(index for index, entry in enumerate(body) if entry.address == advance_address)
assert body[advance].op_str == 'rax, qword ptr [rbx + 8]'
exit_address = body[completion + 3].operands[0].imm
backedge = next(entry for entry in body[advance:] if entry.mnemonic == 'jne' and entry.operands[0].imm < entry.address)
loop = next(index for index, entry in enumerate(body) if entry.address == backedge.operands[0].imm)
assert stage < loop < last_gate and body[loop].op_str == 'rdi, qword ptr [rbx + 0x20]'
assert body[next(index for index, entry in enumerate(body) if entry.address == exit_address)].op_str == 'r12, qword ptr [rsp + 0x88]'
discovery, = sites('?QueueFinaleDiscovery@instance_silver_hand_InstanceMapScript@')
assert advance < discovery and body[discovery - 1].op_str == 'rcx, r15'
assert any(entry.mnemonic == 'je' and entry.operands[0].imm == body[discovery - 1].address for entry in body[stage:last])
assert [(entry.mnemonic, entry.op_str) for entry in body[discovery - 5:discovery - 1:2]] == [('test', 'r12b, r12b'), ('test', 'r14b, r14b')]
for prefix in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@'):
    assert not sites(prefix)
print('PASS selected step10 retains independent r12b/r14b found state, live70yd106777/106719, original Talk3 and one final-cast helper after resolved boss; skips only found actor on later participant, exits after both, queues discovery after empty list or missing counterpart at exhaustion. Full-GUID admission/retries verified by linked proofs; no Player progression/item path added. Persistent epochs/client VFX remain separate.')
