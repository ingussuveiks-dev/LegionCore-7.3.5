"""Verify real497 acknowledgement/rollback and the separate guarded cue caller."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_ready_release.py')))
native, instructions, positions = (previous[name] for name in ('native', 'instructions', 'positions'))
suffix = '@npc_hrutnirAI@npc_hrutnir@@'
body = instructions('?StartMagnarConversation' + suffix)
factory = positions(body, '?CreateConversation@Conversation@@')[0]
constructor = positions(body, '??0Conversation@@')[0]
assert constructor < factory
assert [entry.op_str for entry in body[factory - 3:factory]] == ['r9, r15', 'r8d, 0x1f1', 'rcx, r14']
for operand in ('rsi, rcx', 'r15, qword ptr [rcx + 0x20]', 'r12d, dword ptr [rcx + 0x70]',
                'rbp, qword ptr [rcx + 0x60]', 'qword ptr [rsp + 0x20], 0',
                'qword ptr [rsp + 0x28], rbx', 'rbx, [r15 + 0xd0]'):
    assert any(entry.op_str == operand for entry in body[:factory])
assert any(entry.op_str == 'dword ptr [rax + 8]' and entry.mnemonic == 'lock inc' for entry in body[:factory])
assert body[factory + 1].op_str == 'al, al' and body[factory + 2].mnemonic == 'jne'
success = next(index for index, entry in enumerate(body) if entry.address == body[factory + 2].operands[0].imm)
remove = positions(body, '?Remove@Conversation@@')[0]
rollback = body[remove - 1].address
assert body[remove - 1].op_str == 'rcx, r14'
for operand in ('qword ptr [r15 + 0x9c0], rsi', 'qword ptr [rsi + 0x60], rbp', 'byte ptr [rbp], 0'):
    check = next(index for index, entry in enumerate(body[success:remove], start=success) if entry.mnemonic == 'cmp' and entry.op_str == operand)
    assert body[check + 1].mnemonic == 'jne' and body[check + 1].operands[0].imm == rollback
context = positions(body, '?IsMagnarOfferContext' + suffix)[0]
world = positions(body, '?IsInWorld@Object@@')[0]
assert success < context < world < remove and body[world - 1].op_str == 'rcx, r14'
assert body[context - 2].op_str == 'edx, r12d'
for check in (context, world):
    assert body[check + 1].op_str == 'al, al' and body[check + 2].mnemonic == 'je' and body[check + 2].operands[0].imm == rollback
guid = positions(body, '?GetGuidValue@Object@@')[0]
assert world < guid < remove
assert body[guid - 1].op_str == 'rcx, r14' and body[guid - 2].op_str == 'edx, edx'
assert [entry.op_str for entry in body[guid + 1:guid + 4]] == ['xmm0, xmmword ptr [rax]', 'xmmword ptr [rsi + 0xad], xmm0', 'bl, 1']
deletion = next(index for index, entry in enumerate(body[factory:success], start=factory) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax]')
assert body[deletion - 1].op_str == 'rcx, r14' and body[deletion - 2].op_str == 'edx, 1'


def reachable(entries, start):
    addresses = {entry.address: index for index, entry in enumerate(entries)}
    visited, pending = set(), [start]
    while pending:
        index = pending.pop()
        if index in visited:
            continue
        visited.add(index)
        entry = entries[index]
        if entry.mnemonic == 'ret':
            continue
        if entry.mnemonic.startswith('j'):
            target = entry.operands[0].imm
            assert target in addresses, 'Unreviewed external failure branch'
            pending.append(addresses[target])
            if entry.mnemonic == 'jmp':
                continue
        assert index + 1 < len(entries), 'Failure falls outside native function'
        pending.append(index + 1)
    return visited


false_result = remove + 1
assert body[false_result].mnemonic == 'xor' and body[false_result].op_str == 'bl, bl'
assert any(entry.op_str == 'eax, bl' and entry.mnemonic == 'movzx' for entry in body[false_result:])
failure = reachable(body, factory + 3)
assert deletion in failure and false_result in failure and guid + 3 not in failure
assert false_result in reachable(body, remove - 1) and guid + 3 not in reachable(body, remove - 1)
assert guid + 3 in reachable(body, success)

body = instructions('?MovementInform' + suffix)
creation = positions(body, '?StartMagnarConversation' + suffix)[0]
queue = next(index for index in positions(body, '?AddDelayedEvent@FunctionProcessor@@') if body[index - 1].op_str == 'edx, 0x1770')
assert body[creation - 1].op_str == 'rcx, rbx'
assert body[creation + 1].op_str == 'al, al' and body[creation + 2].mnemonic == 'jne'
accepted = next(index for index, entry in enumerate(body) if entry.address == body[creation + 2].operands[0].imm)
restore = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rbx + 0x5d], 1')
contexts = positions(body, '?IsMagnarOfferContext' + suffix)
assert len(contexts) == 3 and creation < contexts[2] < restore < accepted < queue
for operand in ('rax, rax', 'byte ptr [rax], 0', 'qword ptr [r14 + 0x9c0], rbx', 'qword ptr [rbx + 0x60], rax'):
    check = max(index for index, entry in enumerate(body[:contexts[2]]) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operand)
    branch = 'je' if operand == 'rax, rax' else 'jne'
    assert body[check + 1].mnemonic == branch and body[check + 1].operands[0].imm == body[restore + 1].address
assert body[contexts[2] - 2].op_str == 'edx, r15d'
assert body[contexts[2] + 1].op_str == 'al, al' and body[contexts[2] + 2].mnemonic == 'je' and body[contexts[2] + 2].operands[0].imm == body[restore + 1].address
assert any(entry.mnemonic == 'lock cmpxchg' and entry.op_str == 'dword ptr [rdi + 8], ecx' for entry in body[creation:restore])
assert restore in reachable(body, creation + 3) and queue not in reachable(body, creation + 3)
assert queue in reachable(body, accepted)
assert not positions(body, '?CreateConversation@Unit@@')
print('PASS: installed native497 helper snapshots actor/instance/strong unconsumed token, checks real factory bool and deletes a failed allocation. Current AI/token/context/created-world gates precede publishing only its real GUID and true result; native Remove rollback returns false. Actual caller branches on acknowledgement, failure CFG cannot reach6000ms queue and weak same-token/current-context recovery restores only the original claim. Automatic retry/client delivery remain separate.')
