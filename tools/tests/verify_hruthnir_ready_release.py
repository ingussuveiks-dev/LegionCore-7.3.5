"""Verify current-offer authority on hello, raw selection and publication."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_request_release.py')))
native, instructions, positions = (previous[name] for name in ('native', 'instructions', 'positions'))
suffix = '@npc_hrutnirAI@npc_hrutnir@@'
ready_name = '?IsMagnarOfferReady' + suffix
body = instructions(ready_name)
deny = body[4].operands[0].imm
assert body[2].op_str == 'byte ptr [rcx + 0xac], 0' and body[4].mnemonic == 'je'
for operand, branch in (('rax, rax', 'je'), ('byte ptr [rax], 0', 'je'),
                        ('byte ptr [rcx + 0x5d], 0', 'jne'), ('qword ptr [rcx + 0x60], 0', 'jne')):
    check = next(index for index, entry in enumerate(body) if entry.op_str == operand)
    assert body[check + 1].mnemonic == branch and body[check + 1].operands[0].imm == deny
assert body[5].op_str == 'rax, qword ptr [rcx + 0x78]'
context = positions(body, '?IsMagnarOfferContext' + suffix)
request = positions(body, '?HasMagnarChamberRequest' + suffix)
assert len(context) == len(request) == 1 and context[0] < request[0]
for check in (context[0], request[0]):
    assert body[check + 1].op_str == 'al, al' and body[check + 2].mnemonic == 'je' and body[check + 2].operands[0].imm == deny
first = request[0] + 3
expected = [
    ('mov', 'rax, qword ptr [rbx + 0x20]'), ('movss', 'xmm0, dword ptr [rax + 0xd0]'), ('subss', None),
    ('movss', 'xmm1, dword ptr [rax + 0xd4]'), ('subss', None),
    ('movss', 'xmm2, dword ptr [rax + 0xd8]'), ('subss', None),
    ('mulss', 'xmm0, xmm0'), ('mulss', 'xmm1, xmm1'), ('mulss', 'xmm2, xmm2'),
    ('addss', 'xmm1, xmm0'), ('movss', None), ('addss', 'xmm1, xmm2'), ('comiss', 'xmm0, xmm1')]
for entry, (mnemonic, operand) in zip(body[first:], expected):
    assert entry.mnemonic == mnemonic and (operand is None or entry.op_str == operand)
for index, register, value in ((2, 'xmm0', 4763.73), (4, 'xmm1', 340.87), (6, 'xmm2', -37.19), (11, 'xmm0', 4.0)):
    entry = body[first + index]
    assert entry.reg_name(entry.operands[0].reg) == register
    assert entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP
    address = entry.address + entry.size + entry.operands[1].mem.disp
    assert native['binary'][native['offset'](address):native['offset'](address) + 4] == struct.pack('<f', value)
assert body[first + 14].mnemonic == 'jb' and body[first + 14].operands[0].imm == deny
assert body[first + 15].op_str == 'al, 1'
getter = instructions('?GetData' + suffix)
read = positions(getter, ready_name)
assert len(read) == 1 and getter[3].op_str == 'edx, 0x425' and getter[4].mnemonic == 'jne'
assert getter[read[0] + 1].op_str == 'eax, al' and getter[read[0] + 4].mnemonic == 'ret'
assert getter[4].operands[0].imm == getter[read[0] + 5].address
assert any(entry.op_str == 'edx, 0x423' for entry in getter[read[0] + 5:])
selection = instructions('?sGossipSelect' + suffix)
gate = positions(selection, ready_name)[0]
menu = positions(selection, '?GetItem@GossipMenu@@')[0]
retire = positions(selection, '?reset@?$shared_ptr@_N@std@@')[0]
flag = positions(selection, '?RemoveFlag@Object@@')[0]
assert positions(selection, '?CanStart@HruthnirGossip@@')[0] < gate < menu < retire < flag
assert selection[gate + 1].op_str == 'al, al' and selection[gate + 2].mnemonic == 'je'
assert selection[gate + 2].operands[0].imm > selection[positions(selection, '?MovePoint@MotionMaster@@')[0]].address
assert selection[retire - 1].op_str == 'byte ptr [rsi + 0xac], 0'
hello = instructions('?OnGossipHello@npc_hrutnir@@')
clear = positions(hello, '?ClearMenus@PlayerMenu@@')[0]
close = positions(hello, '?SendCloseGossip@PlayerMenu@@')[0]
reject = hello[clear - 1].address
for operand in ('byte ptr [rbx + 0x858], 0', 'rcx, rcx', 'eax, eax'):
    check = next(index for index, entry in enumerate(hello) if entry.op_str == operand and entry.mnemonic in ('cmp', 'test'))
    assert hello[check + 1].mnemonic == 'je' and hello[check + 1].operands[0].imm == reject
query = next(index for index, entry in enumerate(hello) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x50]')
assert hello[query - 1].op_str == 'edx, 0x425' and query < clear < close
tables = previous['previous']['previous']['previous']['previous']['selected_tables']
assert len(tables('?GetAI@npc_hrutnir@@', ((0x50, '?GetData' + suffix), (0xc8, '?sGossipSelect' + suffix)))) == 1
callback = instructions('??R<lambda_5>@?BL@??MovementInform' + suffix)
publish = next(index for index, entry in enumerate(callback) if entry.op_str == 'byte ptr [rcx + 0xac], 1')
flag = positions(callback, '?SetFlag@Object@@')[0]
assert positions(callback, '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')[0] < positions(callback, '?HasMagnarChamberRequest' + suffix)[1] < publish == flag - 4
reset = instructions('?Reset' + suffix)
zero = next(index for index, entry in enumerate(reset) if entry.op_str == 'byte ptr [rdi + 0xac], bpl')
assert any(entry.mnemonic == 'xor' and entry.op_str == 'ebp, ebp' for entry in reset[:zero])
assert any(entry.op_str == 'edx, 0x7d' for entry in reset[:zero]) and reset[zero - 1].op_str == 'r8d, 1'
assert len(positions(reset, '?RemoveFlag@Object@@')) == 1 and reset[-1].mnemonic == 'ret'
action = instructions('?DoAction' + suffix)
epoch = next(index for index, entry in enumerate(action) if entry.op_str == 'dword ptr [rbx + 0xa8], esi')
erase = positions(action, '?RemoveFlag@Object@@')[0]
assert action[erase - 1].op_str == 'byte ptr [rbx + 0xac], 0' and erase < epoch
assert any(entry.op_str == 'edx, 0x7d' for entry in action[erase - 6:erase]) and action[erase - 4].op_str == 'r8d, 1'
print('PASS: canonical hello queries actual current enabled AI1061; raw selection rechecks ready authority before menu consumption/move. Actual ready/token/claim/live-context/current-epoch/ordered endpoint guards, late publication, exactly-once consumption and gossip-only Reset/epoch clearing verified. Same-epoch physical movement identity,497 ownership and rendered client remain open.')
