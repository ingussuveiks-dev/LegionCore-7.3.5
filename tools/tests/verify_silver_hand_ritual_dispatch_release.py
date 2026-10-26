"""Verify selected step8 actor search continues after an empty first lookup."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
callback = '?onScenarionNextStep@instance_silver_hand_InstanceMapScript@'
body = instructions(callback)
factory = instructions('?GetInstanceScript@instance_silver_hand@@')
tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x118)[0] == native['symbol'](callback):
            tables.append(address)
assert len(tables) == 1
assert any(entry.op_str == 'edx, 8' and entry.mnemonic == 'cmp' for entry in body[:16])
find = native['symbol']('?FindNearestCreature@WorldObject@@')
lookups = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
           and native['resolve'](entry.operands[0].imm) == find]
assert len(lookups) == 3
lookup = lookups[0]
assert [entry.op_str for entry in body[lookup - 4:lookup]] == ['r9b, 1', 'xmm2, xmm6', 'edx, 0x1a0b4', 'rcx, rdi']
radius, = [entry for entry in body[:lookup] if entry.mnemonic == 'movss' and entry.op_str.startswith('xmm6, dword ptr [rip')]
assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 70.0
assert body[lookup + 1].mnemonic == 'test' and body[lookup + 1].op_str == 'rax, rax'
assert body[lookup + 2].mnemonic == 'jne'
dispatch_address = body[lookup + 2].operands[0].imm
dispatch = next(index for index, entry in enumerate(body) if entry.address == dispatch_address)
assert [(entry.mnemonic, entry.op_str) for entry in body[lookup + 3:lookup + 5]] == [
    ('mov', 'rax, qword ptr [rbx + 8]'), ('test', 'rax, rax')]
loopback, = [entry for entry in body[lookup:dispatch] if entry.mnemonic == 'jne'
             and entry.operands[0].type == CS_OP_IMM and entry.operands[0].imm < body[lookup].address]
assert loopback.mnemonic == 'jne'
loop = next(index for index, entry in enumerate(body) if entry.address == loopback.operands[0].imm)
assert body[loop].op_str == 'rdi, qword ptr [rbx + 0x20]' and loop < lookup
contact = native['symbol']('?CanContact@Player@@')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == contact for entry in body[loop:lookup])
assert [(entry.mnemonic, entry.op_str) for entry in body[dispatch:dispatch + 4]] == [
    ('mov', 'rcx, qword ptr [rax + 0x9c0]'), ('mov', 'rax, qword ptr [rcx]'), ('mov', 'edx, 1'), ('call', 'qword ptr [rax + 0x48]')]
assert body[dispatch + 4].mnemonic == 'jmp'
exit_index = next(index for index, entry in enumerate(body) if entry.address == body[dispatch + 4].operands[0].imm)
assert body[exit_index].op_str == 'rbx, qword ptr [rsp + 0xb8]'
assert any(entry.mnemonic == 'ret' for entry in body[exit_index:dispatch])
third = '?DoAction@npc_travar_thirdAI@npc_travar_third@@'
selected = []
for entry in instructions('?GetAI@npc_travar_third@@'):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x48)[0] == native['symbol'](third):
            selected.append(address)
assert len(selected) == 1
for method in ('?SetCurrentStep@Scenario@@', '?UpdateCurrentStep@Scenario@@'):
    assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x118 for entry in instructions(method))
print('PASS: canonical1539 instance factory selects step callback0x118; actual step8 live70yd106676 lookup null fallthrough advances to next participant and loops through CanContact/search, success calls selected third-AI DoAction0x48(1) then exits once. Both native Scenario transition paths dispatch slot0x118. Actor-all-missing replay, step10 search, downstream credit and client traversal remain separate.')
