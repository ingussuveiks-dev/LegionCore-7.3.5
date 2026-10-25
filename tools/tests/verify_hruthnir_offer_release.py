"""Verify the installed point6 offer lifetime without certifying client arrival."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cue_release.py')))
native = previous['native']
instructions = previous['instructions']
positions = previous['positions']
suffix = '@npc_hrutnirAI@npc_hrutnir@@'
context_name = '?IsMagnarOfferContext' + suffix
callback = '??R<lambda_5>@?BJ@??MovementInform' + suffix
bridge = '?_Do_call@?$_Func_impl_no_alloc@V<lambda_5>@?BJ@??MovementInform' + suffix

movement = instructions('?MovementInform' + suffix)
context = positions(movement, context_name)[0]
queue = next(index for index in positions(movement, '?AddDelayedEvent@FunctionProcessor@@')
             if movement[index - 1].op_str == 'edx, 0x1964')
assert context < queue
deny = movement[context + 2].operands[0].imm
assert movement[context + 1].op_str == 'al, al' and movement[context + 2].mnemonic == 'je'
for operand in ('qword ptr [rbx + 0x78], r14', 'byte ptr [rbx + 0x5d], r14b', 'qword ptr [rbx + 0x60], r14'):
    check = next(index for index, entry in enumerate(movement[:context]) if entry.mnemonic == 'cmp' and entry.op_str == operand)
    assert movement[check + 1].mnemonic == 'jne' and movement[check + 1].operands[0].imm == deny
assert any(entry.op_str == 'byte ptr [rcx], r14b' for entry in movement[context:queue])
assert any(entry.mnemonic == 'lea' and entry.op_str == 'rcx, [rbx + 0x78]' for entry in movement[context:queue])
assert any(entry.mnemonic == 'lock inc' and entry.op_str == 'dword ptr [rdi + 0xc]' for entry in movement[context:queue]), 'Offer queue does not capture a weak owner'
assert any(entry.op_str == 'r12d, dword ptr [r15 + 0x3b8]' for entry in movement[context:queue])
assert any(entry.op_str == 'dword ptr [rsp + 0x70], r12d' for entry in movement[context:queue])
assert any(entry.op_str == 'rcx, 0x36a0' for entry in movement[context:queue])
assert any(entry.op_str == 'dl, 0xf' for entry in movement[context:queue]), 'Native Talk15 was lost'
tables = [entry.address + entry.size + entry.operands[1].mem.disp for entry in movement[context:queue]
          if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP]
assert sum(struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](bridge)
           for address in tables) == 1, 'Native point6 queue selects the wrong callback'
address = native['symbol'](bridge)
encoded = native['binary'][native['offset'](address):native['offset'](address) + 9]
assert encoded[:5] == b'\x48\x83\xc1\x08\xe9'
assert native['resolve'](address + 9 + struct.unpack_from('<i', encoded, 5)[0]) == native['symbol'](callback)

body = instructions(callback)
assert any(entry.mnemonic == 'lock cmpxchg' and entry.op_str == 'dword ptr [rdx + 8], ecx' for entry in body)
token = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rdi], 0')
deny = body[token + 1].operands[0].imm
assert body[token - 2].op_str == 'rdi, rdi' and body[token - 1].mnemonic == 'je' and body[token - 1].operands[0].imm == deny
assert body[token + 1].mnemonic == 'jne'
checks = positions(body, context_name)
talk = positions(body, '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')[0]
claim = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rdi], 1')
assert len(checks) == 2 and token < checks[0] < claim < talk < checks[1]
assert any(entry.op_str == 'dl, 0x10' for entry in body[claim:talk]), 'Native Talk16 was lost'
for first, last in ((token, talk), (talk, checks[1])):
    for operand in ('qword ptr [rax + 0x9c0], rcx', 'qword ptr [rcx + 0x78], rdi'):
        check = next(index for index, entry in enumerate(body[first:last], start=first) if entry.mnemonic == 'cmp' and entry.op_str == operand)
        assert body[check + 1].mnemonic == 'jne' and body[check + 1].operands[0].imm == deny, 'Talk Reset/replacement can mutate a revoked offer'
for check in checks:
    assert body[check - 1].op_str == 'edx, dword ptr [rsi + 0x20]'
    assert body[check + 1].op_str == 'al, al' and body[check + 2].mnemonic == 'je' and body[check + 2].operands[0].imm == deny
flag = positions(body, '?SetFlag@Object@@')[0]
react = positions(body, '?SetReactState@Creature@@')[0]
release = positions(body, '?RemoveFlag@Object@@')[0]
assert checks[1] < flag < react < release
assert [entry.op_str for entry in body[flag - 3:flag]] == ['edx, 0x7d', 'r8d, 1', 'rcx, qword ptr [rsi + 0x10]']
assert body[react - 2].op_str == 'dl, 1'
assert [entry.op_str for entry in body[release - 3:release]] == ['edx, 0x61', 'r8d, 0x2000380', 'rcx, qword ptr [rsi + 0x10]']
assert not positions(body, '?SetUInt32Value@Object@@'), 'Offer still overwrites unrelated NPC flags'

body = instructions(context_name)
world = positions(body, '?IsInWorld@Object@@')[0]
deny = body[world + 2].operands[0].imm
for operand, branch in (('dword ptr [rcx + 0xa18], 0', 'jne'), ('byte ptr [rcx + 0x39c5], 0', 'jne'),
                        ('qword ptr [rcx + 8], 0', 'je'), ('eax, 0x178d4', 'jne'),
                        ('dword ptr [rax + 0xe0], 0x5d7', 'jne'), ('dword ptr [rax + 0x3b8], edi', 'jne'),
                        ('byte ptr [rax + 0x858], 0', 'je'), ('qword ptr [rax + 0x9c0], rbx', 'jne'),
                        ('eax, 0x38d', 'jne'), ('eax, 3', 'jne')):
    check = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == operand)
    assert body[check + 1].mnemonic == branch and body[check + 1].operands[0].imm == deny, f'Missing point6 context {operand}'
reset = instructions('?Reset' + suffix)
assert all(any(entry.op_str == f'qword ptr [rsi + {hex(offset)}], rbp' for entry in reset) for offset in (0x78,0x80))
assert sum(entry.mnemonic == 'lock xadd' and entry.op_str == 'dword ptr [rbx + 8], eax' for entry in reset) == 2
selection = instructions('?sGossipSelect' + suffix)
retire = positions(selection, '?reset@?$shared_ptr@_N@std@@')[0]
assert selection[retire - 1].op_str == 'rcx, [rsi + 0x78]' and retire < positions(selection, '?RemoveFlag@Object@@')[0]
destructor = instructions('??_Gnpc_hrutnirAI@npc_hrutnir@@')
cleanup = positions(destructor, '?CancelMagnarIntro' + suffix)[0]
assert any(entry.op_str == 'rdi, rcx' for entry in destructor[:cleanup])
assert any(entry.op_str == 'rbx, qword ptr [rdi + 0x80]' for entry in destructor[cleanup:])
assert sum(entry.mnemonic == 'lock xadd' and entry.op_str == 'dword ptr [rbx + 8], eax' for entry in destructor) == 2
print('PASS: installed native6500ms offer queue owns only weak point6 token; existing offer/start claims block duplicates. Actual callback validates live96468/1495/original-instance/AI/909-3 and token identity before/after Talk16, preserves unrelated flags, and Reset/destructor/selection retire only its token. Motion/arrival/re-entry routing and client visibility remain separate.')
