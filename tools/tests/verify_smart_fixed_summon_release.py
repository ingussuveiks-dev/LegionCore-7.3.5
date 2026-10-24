"""Check opt-in fixed-position creation retry and native loader admission in the PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_orestes_farewell_release.py')))
native = previous['native']
instructions = previous['instructions']
action = instructions('?ProcessAction@SmartScript@@')
case_address = previous['action_case'](50)
case_index = next(index for index, entry in enumerate(action) if entry.address == case_address)


def immediate_target(entry):
    if entry.operands and entry.operands[0].type == CS_OP_IMM:
        return entry.operands[0].imm
    return None


def tests_value(entry, value, displacement=None):
    return entry.mnemonic in {'cmp', 'test'} and any(operand.type == CS_OP_IMM and operand.imm == value for operand in entry.operands) and any(operand.type == CS_OP_MEM and (displacement is None or operand.mem.disp == displacement) for operand in entry.operands)


flag_index = next(index for index in range(case_index, case_index + 12) if tests_value(action[index], 2, 0x60))
flag_branch = action[flag_index + 1]
assert flag_branch.mnemonic in {'je', 'jz'}, 'Unflagged summons do not retain their legacy path'
legacy_address = immediate_target(flag_branch)
fixed = [entry for entry in action[flag_index + 2:] if entry.address < legacy_address]
assert any(tests_value(entry, 8) for entry in fixed[:8]), 'Retry path does not require fixed-position target8'
summon_targets = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith('?SummonGameObject@WorldObject@@')}
invokes = [index for index, entry in enumerate(fixed) if entry.mnemonic == 'call' and immediate_target(entry) is not None and native['resolve'](immediate_target(entry)) in summon_targets]
assert len(invokes) == 1, 'Opt-in path must call the native creation API exactly once'
invoke = invokes[0]
assert fixed[invoke + 1].mnemonic == 'test' and fixed[invoke + 1].op_str == 'rax, rax', 'Creation pointer result is ignored'
by_address = {entry.address: index for index, entry in enumerate(action)}
reject = fixed[invoke + 2]
assert reject.mnemonic in {'je', 'jz'}, 'Null creation does not branch to retained retry'
failure_address = immediate_target(reject)
reset_index = by_address[failure_address]
reset, escape = action[reset_index:reset_index + 2]
assert reset.mnemonic == 'mov' and reset.operands[0].type == CS_OP_MEM and reset.operands[0].mem.disp == 0x99 and reset.operands[0].size == 1, 'Rejected summon does not reset runOnce'
value = reset.operands[1]
if value.type == CS_OP_REG:
    assert reset.reg_name(value.reg) == 'r12b' and any(entry.mnemonic == 'xor' and entry.op_str == 'r12d, r12d' for entry in action[:case_index]), 'Retry reset register is not the native zero value'
else:
    assert value.type == CS_OP_IMM and value.imm == 0, 'Retry reset is not false'
assert escape.mnemonic == 'jmp', 'Failed summon can fall through to LINK'
assert any(entry.mnemonic in {'je', 'jz'} and immediate_target(entry) == failure_address for entry in fixed[:invoke]), 'Missing base object does not retain pending creation'
assert fixed[invoke + 3].mnemonic == 'jmp' and immediate_target(fixed[invoke + 3]) != failure_address, 'Successful creation does not bypass retry reset'
pending = [immediate_target(escape)]
visited = set()
event_dispatch = native['symbol']('?ProcessEvent@SmartScript@@')
returned = False
while pending:
    address = pending.pop()
    if address in visited:
        continue
    visited.add(address)
    assert address in by_address, 'Failure cleanup leaves the inspected function'
    index = by_address[address]
    entry = action[index]
    if entry.mnemonic == 'ret':
        returned = True
        continue
    if entry.mnemonic == 'call' and immediate_target(entry) is not None:
        assert native['resolve'](immediate_target(entry)) != event_dispatch, 'Failure still dispatches a dependent LINK'
    if entry.mnemonic.startswith('j'):
        assert immediate_target(entry) is not None
        pending.append(immediate_target(entry))
        if entry.mnemonic == 'jmp':
            continue
    pending.append(action[index + 1].address)
assert returned, 'Retry failure does not return normally'

loader = instructions('?IsEventValid@SmartAIMgr@@')
flags = [index for index, entry in enumerate(loader) if tests_value(entry, 2, 0x60)]
assert len(flags) == 1, 'Actual loader lacks the summon retry bit2 admission'
admission = loader[flags[0]:flags[0] + 32]
for value in [8, 60, 1]:
    assert any(tests_value(entry, value) for entry in admission), f'Loader lacks retry target/event/repeat requirement{value}'
assert any(tests_value(entry, 0) for entry in admission), 'Loader lacks the nonzero phase-mask requirement'
print('PASS: actual Release gates summon retry on action-param3 bit2 and position8, consumes native creation success, restores rejected one-shot before returning without LINK; loader requires phased non-repeatable UPDATE while legacy summons retain their path')
