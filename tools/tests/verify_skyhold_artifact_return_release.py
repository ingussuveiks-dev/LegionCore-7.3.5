"""Inspect the per-recipient Skyhold gate in the actual native quest counter sink."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions('?KilledMonsterCredit@Player@@')
positions = {item.address: index for index, item in enumerate(body)}


def locate(mnemonic, operands, first=0, last=None):
    return next(index for index in range(first, len(body) if last is None else last)
                if body[index].mnemonic == mnemonic and body[index].op_str == operands)


def target(index):
    return positions[body[index].operands[0].imm]


def calls(prefix, first, last):
    address = native['symbol'](prefix)
    return [index for index in range(first, last) if body[index].mnemonic == 'call'
            and body[index].operands[0].type == CS_OP_IMM
            and native['resolve'](body[index].operands[0].imm) == address]


gate = locate('cmp', 'ebx, 0x1953b')
assert body[gate + 1].mnemonic == 'jne'
ordinary = target(gate + 1)
assert body[ordinary].op_str == 'dword ptr [r12], 3'
for identity, item, acquisition in ((39191, 128289, 94851), (40043, 128908, 98033)):
    switch = locate('cmp', f'ecx, {hex(identity)}', gate, gate + 12)
    assert body[switch + 1].mnemonic == 'je'
    selected = target(switch + 1)
    assert body[selected].op_str == f'ebx, {hex(item)}'
    assert body[selected + 1].op_str == f'r15d, {hex(acquisition)}'
arms = locate('cmp', 'ecx, 0xa091', gate, gate + 12)
assert body[arms + 1].mnemonic == 'jne' and target(arms + 1) == ordinary
assert body[arms + 2].op_str == 'ebx, 0x1f78e' and body[arms + 3].op_str == 'r15d, 0x192ca'
start = target(arms + 4)
world = calls('?IsInWorld@Object@@', start, ordinary)[0]
assert body[world + 1].op_str == 'al, al' and body[world + 2].mnemonic == 'je'
skip = target(world + 2)


def reject(position, branch, destination=skip):
    assert body[position + 1].mnemonic == branch and target(position + 1) == destination


for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'),
                       ('?GetQuestRewardStatus@Player@@', 'jne'), ('?HasItemCount@Player@@', 'je')):
    position = calls(prefix, start, ordinary)[0]
    assert body[position + 1].op_str == 'al, al'
    reject(position + 1, branch)
for operands, branch in (('qword ptr [rsi + 8], 0', 'je'), ('dword ptr [rax + 0xe0], 0x5c7', 'jne'),
                         ('byte ptr [rsi + 0x6848], 0', 'jne'), ('byte ptr [rsi + 0x6849], 0', 'jne')):
    reject(locate('cmp', operands, start, ordinary), branch)
klass = calls('?GetByteValue@Object@@', start, ordinary)[0]
assert any(item.op_str == 'edx, 0x35' for item in body[klass - 3:klass])
assert any(item.op_str == 'r8b, 1' for item in body[klass - 3:klass])
assert body[klass + 1].op_str == 'al, 1'
reject(klass + 1, 'jne')
owned = calls('?HasItemCount@Player@@', start, ordinary)[0]
assert all(any(item.op_str == value for item in body[owned - 4:owned]) for value in ('edx, ebx', 'r8d, 1', 'r9b, 1'))
loop = locate('cmp', 'eax, 0x1953b', owned, ordinary)
assert body[loop + 1].mnemonic == 'jne' and body[loop + 2].op_str == 'r14b, 1'
increment = target(loop + 3)
optional = locate('test', 'byte ptr [rbx + 0x14], 4', loop, ordinary)
reject(optional, 'jne', increment)
assert any(item.op_str == 'eax, r15d' and item.mnemonic == 'cmp' for item in body[optional:ordinary])
assert any(item.op_str == 'edi, eax' and item.mnemonic == 'cmove' for item in body[optional:ordinary])
complete = calls('?HasQuestObjectiveComplete@Player@@', optional, ordinary)[0]
assert body[complete - 3].op_str == 'r8, rbx' and body[complete - 2].op_str == 'rdx, r13'
assert body[complete + 1].op_str == 'al, al' and body[complete + 2].mnemonic == 'je'
restore = target(complete + 2)
assert body[increment].op_str == 'rbx, 0x60'
assert body[increment + 2].mnemonic == 'jne' and target(increment + 2) == loop - 1
acquired = locate('test', 'dil, dil', increment, ordinary)
assert body[acquired + 1].mnemonic == 'je'
missing = target(acquired + 1)
returned = locate('test', 'r14b, r14b', acquired, ordinary)
assert body[returned + 2].mnemonic == 'je' and target(returned + 2) == skip
assert body[restore + 1].mnemonic == 'jmp' and target(restore + 1) == missing
assert missing < skip and body[skip + 2].mnemonic == 'inc' and body[skip + 3].op_str == 'ebx, dword ptr [rsp + 0x60]'
assert body[skip + 4].mnemonic == 'jmp' and target(skip + 4) < gate
write = calls('?SetQuestObjectiveData@Player@@', ordinary, len(body))
assert len(write) == 1 and ordinary < write[0] < missing
group = native['calls']('?RewardPlayerAndGroupAtEvent@Player@@')
assert native['symbol']('?KilledMonsterCredit@Player@@') in group
assert native['symbol']('?IsAtGroupRewardDistance@Player@@') in group
print('PASS: deployed native103739 sink scopes39191/40043/41105, exact artifact/acquisition pairs, live1479/class/quest/inventory gates and all required native objective checks before counter writes. Optional objectives skip, acquisition/return identities are required, rejection advances the quest loop and native per-member group dispatch remains. No client arrival or visual proof.')
