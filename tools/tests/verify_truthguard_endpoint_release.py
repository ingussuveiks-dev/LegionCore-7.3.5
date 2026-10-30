"""Verify installed native-result retry/latches rather than an assumed successful teleport."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_rider_release.py')))
native = previous['native']
instructions = previous['instructions']
find = previous['find']
call_index = previous['call_index']
suffix = previous['suffix']
body = instructions('?MovementInform' + suffix)
guard = call_index(body, '?GetRider' + suffix)
for operand in ('byte ptr [rcx + 0x38], 0', 'byte ptr [rcx + 0x39], 0', 'dword ptr [rcx + 0x3c], 0'):
    assert find(body, operand, 'cmp') < guard
quest = call_index(body, '?GetQuestStatus@Player@@')
rewarded = call_index(body, '?GetQuestRewardStatus@Player@@')
active = find(body, 'byte ptr [rbx + 0x39], 1', 'mov')
assert guard < quest < find(body, 'dword ptr [rdi + 0xa18], 0', 'cmp') < rewarded < active
assert find(body, 'eax, 0xfffffffd', 'test') < active and any(entry.op_str == 'edx, 0xa421' for entry in body[guard:quest])
saved = find(body, 'r12d, word ptr [rdi + 0x6ba8]', 'movzx')
assert active < saved < find(body, 'word ptr [rdi + 0x6ba8], r14w', 'mov')
cast_target = native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
casts = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
         and native['resolve'](entry.operands[0].imm) == cast_target]
assert len(casts) == 2 and active < casts[0] < casts[1]
for index, spell in zip(casts, (209908, 210360)):
    assert any(entry.op_str == f'r8d, {hex(spell)}' for entry in body[index - 7:index])
    assert body[index - 1].op_str == 'rcx, rdi' and any(entry.op_str == 'rdx, rdi' for entry in body[index - 5:index])
    assert body[index + 1].op_str == 'r15d, eax'
result = find(body, 'r15d, r15d', 'test')
assert result > casts[-1] and body[result + 1].mnemonic == 'je'
accepted = body[result + 1].operands[0].imm
success = next(index for index, entry in enumerate(body) if entry.address == accepted)
assert body[success].op_str == 'word ptr [rbx + 0x38], 1', 'Accepted/in-progress pair not latched to true/false'
destroy = call_index(body, '?DespawnOrUnsummon@Creature@@')
assert success < destroy and find(body, 'edx, 0x1f4', 'mov') < destroy
restore = find(body, 'word ptr [rdi + 0x6ba8], r12w', 'mov')
assert body[restore - 2].op_str == 'word ptr [rdi + 0x6ba8], r14w' and body[restore - 1].mnemonic == 'jne'
retry = find(body, 'dword ptr [rbx + 0x3c], 0x3e8', 'mov')
assert result < restore < retry < success and body[retry - 1].op_str == 'byte ptr [rbx + 0x39], 0'
assert b'Truthguard travel: endpoint cast rejected with result' in native['binary']
by_address = {entry.address: entry for entry in body}
pending = [body[result + 2].address]
seen = set()
while pending:
    address = pending.pop()
    if address in seen:
        continue
    assert address in by_address
    seen.add(address)
    entry = by_address[address]
    if entry.mnemonic in ('ret', 'int3'):
        continue
    if entry.mnemonic.startswith('j'):
        assert entry.operands[0].type == CS_OP_IMM
        pending.append(entry.operands[0].imm)
        if entry.mnemonic == 'jmp':
            continue
    pending.append(entry.address + entry.size)
assert body[destroy].address not in seen and accepted not in seen, 'Rejected admission reaches success/despawn'
address = native['symbol']('?UpdateAI' + suffix)
update = instructions('?UpdateAI' + suffix)
assert call_index(update, '?UpdateBoardingRequests' + suffix) < find(update, 'eax, dword ptr [rbx + 0x3c]', 'mov')
assert find(update, 'eax, edi', 'cmp') < find(update, 'eax, edi', 'sub') < find(update, 'dword ptr [rbx + 0x3c], 0', 'mov')
tail = next(index for index, entry in enumerate(update) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x1a8]')
assert update[tail - 2].op_str == 'dword ptr [rbx + 0x3c], 0' and update[tail - 1].op_str == 'rcx, rbx'
assert any(entry.op_str == 'edx, 2' for entry in update[:tail]) and any(entry.op_str == 'r8d, 4' for entry in update[:tail])
slots = previous['tables'][0]
assert slots.count(address) == 1 and slots[0x18 // 8] == address
tick = instructions('?Update@Creature@@')
dispatch = next(index for index, entry in enumerate(tick) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x18]')
assert [entry.op_str for entry in tick[dispatch - 3:dispatch - 1]] == ['rcx, qword ptr [rsi + 0x9c0]', 'rax, qword ptr [rcx]'], 'Native Creature update does not load the real AI vtable'
argument = tick[dispatch - 1]
assert argument.mnemonic == 'mov' and argument.reg_name(argument.operands[0].reg) == 'edx' and argument.operands[1].type == CS_OP_REG, 'Native AI update lost its diff argument'
saved_diff = argument.operands[1].reg
saved = [i for i, entry in enumerate(tick[:32]) if entry.mnemonic == 'mov' and len(entry.operands) == 2 and
         entry.operands[0].type == CS_OP_REG and entry.operands[0].reg == saved_diff and
         entry.operands[1].type == CS_OP_REG and entry.reg_name(entry.operands[1].reg) == 'edx']
assert len(saved) == 1, 'AI diff is not the saved incoming Creature::Update argument'
name = argument.reg_name(saved_diff)
alias = {'ebx':'rbx', 'ebp':'rbp', 'esi':'rsi', 'edi':'rdi'}.get(name, name[:-1] if name.endswith('d') else name)
assert alias in {'rbx','rbp','rsi','rdi','r12','r13','r14','r15'}, 'Diff must survive earlier native calls in a preserved register'
# Creature throttling intentionally adds its previously deferred diff. Accept
# that exact load/add/save sequence, rather than requiring the raw input forever.
accumulated = [i for i, entry in enumerate(tick) if i > saved[0] and entry.mnemonic == 'mov' and
               entry.op_str == f'{name}, edx']
assert len(accumulated) == 1
accumulate = accumulated[0]
assert [entry.op_str for entry in tick[accumulate - 2:accumulate]] == ['edx, dword ptr [rsi + 0x39c8]', f'edx, {name}']
assert tick[accumulate - 2].mnemonic == 'mov' and tick[accumulate - 1].mnemonic == 'add'
assert tick[accumulate + 1].mnemonic == 'mov' and tick[accumulate + 1].op_str == 'dword ptr [rsi + 0x39c8], edx'
# Follow control flow: another return path's epilogue can precede this dispatch
# in address order, and legitimately restores the same nonvolatile register.
tick_by_address = {entry.address: entry for entry in tick}
pending = [(tick[saved[0] + 1].address, False)]
seen = set()
reached_dispatch = False
while pending:
    address, overwritten = pending.pop()
    if (address, overwritten) in seen:
        continue
    seen.add((address, overwritten))
    if address == tick[dispatch].address:
        assert not overwritten, 'Saved native diff is overwritten on a path reaching AI dispatch'
        reached_dispatch = True
        continue
    entry = tick_by_address[address]
    if address != tick[accumulate].address:
        overwritten |= any(entry.reg_name(register) in {name, alias} for register in entry.regs_access()[1])
    if entry.mnemonic in ('ret', 'int3'):
        continue
    if entry.mnemonic.startswith('j'):
        assert entry.operands[0].type == CS_OP_IMM
        pending.append((entry.operands[0].imm, overwritten))
        if entry.mnemonic == 'jmp':
            continue
    pending.append((entry.address + entry.size, overwritten))
assert reached_dispatch, 'Saved Creature diff cannot reach the native AI update'
factory = instructions('?GetAI@npc_sr_argent_hippogryph@@')
assert any(entry.op_str == 'word ptr [rdi + 0x38], 0' for entry in factory)
timer = find(factory, 'dword ptr [rdi + 0x3c], eax', 'mov')
assert factory[timer - 1].op_str == 'eax, eax' and factory[timer - 1].mnemonic == 'xor'
print('PASS: installed selected AI publishes in-progress before native self-casts, branches on their real result, conditionally rolls back selector and queues1000ms retry without despawn; accepted admission latches once. Runtime-bounded UpdateAI polls boarding and clears timer before the selected waypoint slot; actual teleport/credit delivery and client remain separate.')
