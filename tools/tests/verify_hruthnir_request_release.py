"""Verify chamber request authority and retirement in the canonical executable."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_home_release.py')))
native = previous['previous']['native']
instructions, positions = (previous[name] for name in ('instructions', 'positions'))
suffix = '@npc_hrutnirAI@npc_hrutnir@@'
body = instructions('?HasMagnarChamberRequest' + suffix)
assert body[2].op_str == 'byte ptr [rcx + 0xa5], 0' and body[2].mnemonic == 'cmp'
assert body[4].mnemonic == 'jne' and body[5].op_str == 'al, al' and body[8].mnemonic == 'ret'
assert body[4].operands[0].imm == body[9].address
lookup = positions(body, '?GetInstanceScript@WorldObject@@')[0]
assert body[lookup - 2].op_str == 'rcx, qword ptr [rcx + 0x20]'
assert body[lookup + 2].op_str == 'rax, rax' and body[lookup + 3].mnemonic == 'je'
deny = body[lookup + 3].operands[0].imm
credit = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.op_str == 'r8')
epoch = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.op_str == 'rax')
assert lookup < credit < epoch
assert body[credit - 3].op_str == 'edx, 0x423' and body[credit - 2].op_str == 'r8, qword ptr [rcx + 0xb0]'
assert body[credit + 1].op_str == 'eax, eax' and body[credit + 2].mnemonic == 'je' and body[credit + 2].operands[0].imm == deny
assert [entry.op_str for entry in body[epoch - 4:epoch]] == ['edx, 0x424', 'ebx, dword ptr [rbx + 0xa8]', 'rax, qword ptr [rcx + 0xb0]', 'rcx, rdi']
assert body[epoch + 1].op_str == 'eax, ebx' and body[epoch + 2].mnemonic == 'jne' and body[epoch + 2].operands[0].imm == deny
assert body[epoch + 4].op_str == 'al, 1'
failure = next(index for index, entry in enumerate(body) if entry.address == deny)
assert body[failure + 1].op_str == 'al, al' and body[failure + 4].mnemonic == 'ret'
action = instructions('?DoAction' + suffix)
cancel = positions(action, '?CancelMagnarIntro' + suffix)[0]
retirements = positions(action, '?reset@?$shared_ptr@_N@std@@')
assert len(retirements) == 2 and cancel < retirements[0] < retirements[1]
assert [action[index - 1].op_str for index in retirements] == ['rcx, [rbx + 0x78]', 'rcx, [rbx + 0x60]']
epoch_store = next(index for index, entry in enumerate(action) if entry.op_str == 'dword ptr [rbx + 0xa8], esi')
for operand in ('byte ptr [rbx + 0x5d], 0', 'dword ptr [rbx + 0x70], 0', 'byte ptr [rbx + 0xa5], 0', 'byte ptr [r14], 0'):
    assert any(entry.mnemonic == 'mov' and entry.op_str == operand for entry in action[retirements[1]:epoch_store])
epoch_compare = next(index for index, entry in enumerate(action) if entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rbx + 0xa8], eax')
assert epoch_compare < cancel < epoch_store < positions(action, '?IsMagnarOfferContext' + suffix)[0]
assert action[epoch_compare + 1].mnemonic == 'je' and action[epoch_compare + 1].operands[0].imm == action[epoch_store + 2].address
print('PASS: installed point6 and both offer callbacks require the acknowledged native chamber request/current1060 epoch. Actual helper rejects absent claim/instance/1059 credit/epoch mismatch; epoch transition retires both weak queue owners and arrival snapshot before rearming. Same-epoch movement identity, created497 cleanup and client traversal remain separate.')
