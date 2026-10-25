"""Verify native queued Hruthnir cue weak ownership, claim and reset lifetime."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_gossip_release.py')))
native = previous['native']
instructions = previous['instructions']
positions = previous['positions']
suffix = '@npc_hrutnirAI@npc_hrutnir@@'
movement = '?MovementInform' + suffix
reset = '?Reset' + suffix
destructor = '??_Gnpc_hrutnirAI@npc_hrutnir@@'
callback = '??R<lambda_6>@?BM@??MovementInform' + suffix
bridge = '?_Do_call@?$_Func_impl_no_alloc@V<lambda_6>@?BM@??MovementInform' + suffix

selection = instructions('?sGossipSelect' + suffix)
flag = positions(selection, '?RemoveFlag@Object@@')[0]
instance = next(index for index, entry in enumerate(selection[:flag]) if entry.op_str == 'dword ptr [rsi + 0x70], ecx')
assert selection[instance - 1].op_str == 'ecx, dword ptr [rax + 0x3b8]' and selection[instance - 2].op_str == 'rax, qword ptr [rsi + 0x20]', 'Original selection instance not retained'
assert any(entry.op_str == 'byte ptr [rcx], 0' for entry in selection[:flag]), 'Fresh cue token must start unconsumed'
assignment = positions(selection, '??4?$shared_ptr@_N@std@@QEAAAEAV01@$$QEAV01@@Z')[0]
assert any(entry.mnemonic == 'lea' and entry.op_str == 'rcx, [rsi + 0x60]' for entry in selection[instance:assignment]), 'Fresh cue token assignment lost actual owner field'
assert any(entry.op_str == 'dword ptr [rax + 8], 1' for entry in selection[instance:assignment])
assert any(entry.op_str == 'dword ptr [rax + 0xc], 1' for entry in selection[instance:assignment])
transfer = instructions('??4?$shared_ptr@_N@std@@QEAAAEAV01@$$QEAV01@@Z')
for operand in ('rax, qword ptr [rdx]', 'r8, qword ptr [rdx + 8]', 'qword ptr [rdi], rax', 'qword ptr [rdi + 8], r8'):
    assert any(entry.mnemonic == 'mov' and entry.op_str == operand for entry in transfer), 'Actual shared-owner assignment bridge changed'
assert selection[flag - 1].op_str == 'byte ptr [rsi + 0x5d], 1', 'Movement not armed before external flag/motion calls'

body = instructions(movement)
conversation = next(index for index in positions(body, '?CreateConversation@Unit@@') if body[index - 2].op_str == 'edx, 0x1f1')
pending = max(index for index, entry in enumerate(body[:conversation]) if entry.op_str == 'byte ptr [rbx + 0x5d], r14b' and entry.mnemonic == 'cmp')
queue = next(index for index in positions(body, '?AddDelayedEvent@FunctionProcessor@@') if body[index - 1].op_str == 'edx, 0x1770')
assert body[pending + 1].mnemonic == 'je' and body[pending + 1].operands[0].imm > body[queue].address
assert body[pending + 2].op_str == 'byte ptr [rbx + 0x5d], r14b'
assert any(entry.mnemonic == 'xor' and entry.op_str == 'r14d, r14d' for entry in body[:pending]), 'Arrival consume is not zero'
assert pending < conversation < queue
assert any(entry.op_str == 'r14, qword ptr [rbx + 0x20]' for entry in body[pending:conversation])
assert any(entry.op_str == 'r15d, dword ptr [rbx + 0x70]' for entry in body[pending:conversation])
assert any(entry.op_str == 'qword ptr [rsp + 0x60], r14' for entry in body[conversation:queue])
assert any(entry.op_str == 'dword ptr [rsp + 0x68], r15d' for entry in body[conversation:queue])
assert any(entry.op_str == 'rcx, 0x36a0' for entry in body[conversation:queue]), 'Native Unit function queue replaced'
assert any(entry.mnemonic == 'lock inc' and entry.op_str == 'dword ptr [rdi + 0xc]' for entry in body[pending:conversation]), 'Capture lost weak-only ownership'
tables = [entry.address + entry.size + entry.operands[1].mem.disp for entry in body[conversation:queue]
          if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP]
assert sum(struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](bridge)
           for address in tables) == 1, 'Native std::function does not select cue bridge'
address = native['symbol'](bridge)
encoded = native['binary'][native['offset'](address):native['offset'](address) + 9]
assert encoded[:5] == b'\x48\x83\xc1\x08\xe9'
assert native['resolve'](address + 9 + struct.unpack_from('<i', encoded, 5)[0]) == native['symbol'](callback)

body = instructions(callback)
assert any(entry.mnemonic == 'lock cmpxchg' and entry.op_str == 'dword ptr [rdx + 8], ecx' for entry in body), 'Weak lock does not acquire a live token'
token = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rax], 0')
assert body[token - 2].op_str == 'rax, rax' and body[token - 1].mnemonic == 'je'
assert body[token + 1].mnemonic == 'jne' and body[token + 2].op_str == 'byte ptr [rax], 1'
deny = body[token + 1].operands[0].imm
assert body[token - 1].operands[0].imm == deny
for operands, branch in (('dword ptr [rcx + 0xa18], 0', 'jne'), ('byte ptr [rcx + 0x39c5], 0', 'jne'),
                         ('eax, 0x178d4', 'jne'), ('dword ptr [rcx + 0xe0], 0x5d7', 'jne'),
                         ('dword ptr [rcx + 0x3b8], eax', 'jne'), ('rax, rax', 'je'),
                         ('byte ptr [rcx + 0x858], 0', 'je'), ('qword ptr [rcx + 0x9c0], rax', 'jne'),
                         ('byte ptr [rax + 0x858], 0', 'je'), ('rcx, rcx', 'je'), ('byte ptr [rax + 0x39c5], 0', 'jne')):
    check = next(index for index, entry in enumerate(body) if index > token and entry.mnemonic in ('cmp', 'test') and entry.op_str == operands)
    assert body[check + 1].mnemonic == branch and body[check + 1].operands[0].imm == deny, f'Missing cue lifetime gate: {operands}'
identity = next(index for index, entry in enumerate(body) if entry.op_str == 'qword ptr [rcx + 0x9c0], rax')
assert body[identity - 1].op_str == 'rax, qword ptr [rdi + 0x20]', 'Expected AI is not identity-only capture'
world = positions(body, '?IsInWorld@Object@@')[0]
assert world > token and body[world + 2].mnemonic == 'je' and body[world + 2].operands[0].imm == deny
lookup = positions(body, '?FindNearestCreature@WorldObject@@')[0]
assert body[lookup - 1].op_str == 'edx, 0x17722' and body[lookup - 3].op_str == 'r9b, 1'
distance = body[lookup - 2]
assert distance.mnemonic == 'movss' and distance.operands[1].type == CS_OP_MEM and distance.operands[1].mem.base == X86_REG_RIP
assert struct.unpack_from('<f', native['binary'], native['offset'](distance.address + distance.size + distance.operands[1].mem.disp))[0] == 60.0
action = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x48)
assert body[action - 1].op_str == 'edx, 1' and action > lookup > token

body = instructions(reset)
assert any(entry.op_str == 'byte ptr [rsi + 0x5d], 0' for entry in body)
assert any(entry.op_str == 'qword ptr [rsi + 0x60], rbp' for entry in body)
assert any(entry.op_str == 'qword ptr [rsi + 0x68], rbp' for entry in body)
assert any(entry.mnemonic == 'lock xadd' and entry.op_str == 'dword ptr [rbx + 8], eax' for entry in body)
assert not positions(body, '?KillAllFunctions@FunctionProcessor@@'), 'Reset must not cancel unrelated Unit delayed actions'
body = instructions(destructor)
assert any(entry.op_str == 'rbx, qword ptr [rdi + 0x68]' for entry in body)
assert any(entry.mnemonic == 'lock xadd' and entry.op_str == 'dword ptr [rbx + 8], eax' for entry in body), 'Actual AI destructor retains token ownership'
assert len(previous['selected_tables']('?GetAI@npc_hrutnir@@', ((0, destructor), (0x28, reset), (0x1a8, movement)))) == 1
print('PASS: installed Hruthnir arrival claim, actual497/native6000ms std::function bridge and weak-token callback, original-instance/live/AI guards, consume-before-action1, Reset/destructor invalidation and real factory slots. No automatic recovery,497 delivery or client/VFX certification.')
