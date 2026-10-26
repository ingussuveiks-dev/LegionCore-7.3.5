"""Check actual exit aura/recovered-victory branches and exactly-once GO credit."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
method = '?OnGossipHello@go_azsuna_soul_gem@@'
body = instructions(method)
gate = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == 240012)
assert body[gate + 1].mnemonic == 'jne'
end = body[gate + 1].operands[0].imm
exit_body = [entry for entry in body[gate + 2:] if entry.address < end]
objectives = native['symbol']('?GetQuestObjectiveData@Player@@QEBAHII')
credit = native['symbol']('?QuestObjectiveSatisfy@Player@@')
cleanup = native['symbol']('?Cleanup@AzsunaScythe@@')
reads = [(index, entry) for index, entry in enumerate(exit_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == objectives]
assert len(reads) == 3, 'Exit must read actual entry, victory and prior exit credit'
for (index, entry), identity, branch in zip(reads, (237017, 89673, 240012), ('je', 'je', 'jne')):
    assert any(value.op_str == f'r8d, {hex(identity)}' for value in exit_body[index - 3:index])
    assert any(value.op_str == f'edx, {hex(37660)}' for value in exit_body[index - 3:index])
    assert exit_body[index + 1].op_str == 'eax, eax' and exit_body[index + 2].mnemonic == branch
    if identity != 240012:
        assert exit_body[index + 2].operands[0].imm == end, 'Incomplete recovery must not dispatch exit/cleanup'
credit_site = next(index for index, entry in enumerate(exit_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == credit)
cleanup_site = next(index for index, entry in enumerate(exit_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == cleanup)
assert reads[2][0] < credit_site < cleanup_site
assert exit_body[reads[2][0] + 2].operands[0].imm <= exit_body[cleanup_site].address
assert exit_body[reads[2][0] + 2].operands[0].imm > exit_body[credit_site].address, 'Repeated exit still emits credit'
assert any(entry.mnemonic == 'jne' and entry.operands[0].type == CS_OP_IMM and entry.operands[0].imm == exit_body[reads[2][0] - 3].address for entry in exit_body[:reads[0][0]]), 'Existing aura path must bypass only recovered-entry/victory checks'
tables = []
for entry in instructions('?AddSC_azsuna_scythe@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            at = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<3Q', native['binary'], at)
            if slots[0x10 // 8] == native['symbol'](method):
                tables.append(at)
assert len(tables) == 1, 'Registered gameobject script misses actual repaired callback'
print(f'PASS {native["args"].release}: registered exit callback checks real37660 entry237017/victory89673 when screen aura absent, credits240012 only once before native cleanup. Scope/full flow are native-host tests; no client traversal proof.')
