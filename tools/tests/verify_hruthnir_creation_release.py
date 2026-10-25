"""Verify real497 creation acknowledgement before the native6000ms cue."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_ready_release.py')))
native, instructions, positions = (previous[name] for name in ('native', 'instructions', 'positions'))
body = instructions('?MovementInform@npc_hrutnirAI@npc_hrutnir@@')
factory = positions(body, '?CreateConversation@Conversation@@')[0]
constructor = positions(body, '??0Conversation@@')[0]
queue = next(index for index in positions(body, '?AddDelayedEvent@FunctionProcessor@@') if body[index - 1].op_str == 'edx, 0x1770')
assert constructor < factory < queue
assert [entry.op_str for entry in body[factory - 3:factory]] == ['r9, rbx', 'r8d, 0x1f1', 'rcx, r12']
assert any(entry.op_str == 'qword ptr [rsp + 0x20], 0' for entry in body[constructor:factory])
assert any(entry.op_str == 'qword ptr [rsp + 0x28], rbx' for entry in body[constructor:factory])
assert any(entry.op_str == 'rbx, 0xd0' for entry in body[constructor:factory])
assert body[factory + 1].op_str == 'al, al' and body[factory + 2].mnemonic == 'jne'
success = next(index for index, entry in enumerate(body) if entry.address == body[factory + 2].operands[0].imm)
remove = positions(body, '?Remove@Conversation@@')[0]
rollback = body[remove - 1].address
assert body[remove - 1].op_str == 'rcx, r12' and queue < remove
for operand in ('qword ptr [rbx + 0x9c0], rdi', 'qword ptr [rdi + 0x60], r13', 'byte ptr [r13], 0'):
    check = next(index for index, entry in enumerate(body[success:queue], start=success) if entry.mnemonic == 'cmp' and entry.op_str == operand)
    assert body[check + 1].mnemonic == 'jne' and body[check + 1].operands[0].imm == rollback
assert any(entry.op_str == 'r13, qword ptr [rdi + 0x60]' for entry in body[:factory])
assert any(entry.op_str == 'dword ptr [rax + 8]' and entry.mnemonic == 'lock inc' for entry in body[:factory])
contexts = positions(body, '?IsMagnarOfferContext@npc_hrutnirAI@npc_hrutnir@@')
assert len(contexts) == 4 and factory < contexts[2] < queue < contexts[3]
world = positions(body, '?IsInWorld@Object@@')[0]
assert contexts[2] < world < queue and body[world - 1].op_str == 'rcx, r12'
for check in (contexts[2], world):
    assert body[check + 1].op_str == 'al, al' and body[check + 2].mnemonic == 'je' and body[check + 2].operands[0].imm == rollback
assert body[contexts[2] - 2].op_str == 'edx, r13d'
assert any(entry.op_str == 'r13d, dword ptr [rsp + 0x40]' for entry in body[factory:contexts[2]])
deletion = next(index for index, entry in enumerate(body[factory:success], start=factory) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax]')
assert body[deletion - 1].op_str == 'rcx, r12' and body[deletion - 2].op_str == 'edx, 1'
restore = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rdi + 0x5d], 1')
assert remove < contexts[3] < restore
for operand in ('rax, rax', 'byte ptr [rax], 0', 'qword ptr [rbx + 0x9c0], rdi', 'qword ptr [rdi + 0x60], rax'):
    check = max(index for index, entry in enumerate(body[:contexts[3]]) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operand)
    branch = 'je' if operand == 'rax, rax' else 'jne'
    assert body[check + 1].mnemonic == branch and body[check + 1].operands[0].imm == body[restore + 1].address
assert body[contexts[3] + 1].op_str == 'al, al' and body[contexts[3] + 2].mnemonic == 'je' and body[contexts[3] + 2].operands[0].imm == body[restore + 1].address
assert any(entry.mnemonic == 'lock cmpxchg' and entry.op_str == 'dword ptr [r14 + 8], ecx' for entry in body[remove:restore])

def reachable(start):
    addresses = {entry.address: index for index, entry in enumerate(body)}
    visited, pending = set(), [start]
    while pending:
        index = pending.pop()
        if index in visited:
            continue
        visited.add(index)
        entry = body[index]
        if entry.mnemonic == 'ret':
            continue
        if entry.mnemonic.startswith('j'):
            target = entry.operands[0].imm
            assert target in addresses, 'Unreviewed external failure branch'
            pending.append(addresses[target])
            if entry.mnemonic == 'jmp':
                continue
        assert index + 1 < len(body), 'Failure falls outside native function'
        pending.append(index + 1)
    return visited

failure = reachable(factory + 3)
assert deletion in failure and restore in failure and queue not in failure, 'Failed497 can still queue combat or loses valid retry'
assert queue in reachable(success), 'Acknowledged497 no longer reaches its native cue'
assert queue not in reachable(remove - 1), 'Post-creation rollback can still queue combat'
assert not positions(body, '?CreateConversation@Unit@@'), 'Point7 still uses the unacknowledged void wrapper'
print('PASS: actual installed point7 constructs native497, branches on factory bool and deletes failed allocation without any path to the6000ms cue. Real AI/current unconsumed token/context/created-object-in-world checks dominate success; new-object native Remove handles revocation, while weak same-token/current-context recovery restores only the original pending claim. Actual CFG rollback/queue separation checked. No automatic retry, later497 lifetime or client delivery certificate.')
