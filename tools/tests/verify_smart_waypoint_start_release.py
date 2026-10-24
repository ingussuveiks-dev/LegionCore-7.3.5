"""Verify native waypoint rejection and one-shot/LINK recovery in the deployed PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_orestes_farewell_release.py')))
native = previous['native']
instructions = previous['instructions']
action = instructions('?ProcessAction@SmartScript@@')
start = instructions('?StartPath@SmartAI@@')
start_address = native['symbol']('?StartPath@SmartAI@@')
names = [name for name in native['symbols'] if name.startswith('?StartPath@SmartAI@@')]
assert len(names) == 1 and '@@QEAA_N' in names[0], 'Deployed StartPath is not the bool result API'


def direct_target(entry):
    if entry.operands and entry.operands[0].type == CS_OP_IMM:
        return entry.operands[0].imm
    return None


def call_index(body, prefix):
    target = native['symbol'](prefix)
    found = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and direct_target(entry) is not None and native['resolve'](direct_target(entry)) == target]
    assert len(found) == 1, f'Expected one actual call to {prefix}'
    return found[0]


case_address = previous['action_case'](53)
case_index = next(index for index, entry in enumerate(action) if entry.address == case_address)
invoke = call_index(action, '?StartPath@SmartAI@@')
assert case_index < invoke < case_index + 70, 'WP_START does not reach checked StartPath'
test, branch, reset, escape = action[invoke + 1:invoke + 5]
assert test.mnemonic == 'test' and test.op_str == 'al, al', 'Waypoint result is ignored'
assert branch.mnemonic in {'jne', 'jnz'} and direct_target(branch) == action[invoke + 5].address, 'Successful waypoint does not bypass retry reset'
assert reset.mnemonic == 'mov' and reset.operands[0].type == CS_OP_MEM and reset.operands[0].size == 1 and reset.operands[0].mem.disp == 0x99 and reset.operands[1].type == CS_OP_REG and reset.reg_name(reset.operands[1].reg) == 'al', 'Rejected action does not restore runOnce=false from its zero result'
assert escape.mnemonic == 'jmp', 'Rejected waypoint falls through into quest/LINK actions'

by_address = {entry.address: index for index, entry in enumerate(action)}
pending = [direct_target(escape)]
visited = set()
link_address = native['symbol']('?ProcessEvent@SmartScript@@')
found_return = False
while pending:
    address = pending.pop()
    if address in visited:
        continue
    visited.add(address)
    assert address in by_address, 'Failure cleanup leaves the inspected function'
    position = by_address[address]
    entry = action[position]
    if entry.mnemonic == 'ret':
        found_return = True
        continue
    if entry.mnemonic == 'call' and direct_target(entry) is not None:
        assert native['resolve'](direct_target(entry)) != link_address, 'Failure can still dispatch a LINK event'
    if entry.mnemonic.startswith('j'):
        assert direct_target(entry) is not None
        pending.append(direct_target(entry))
        if entry.mnemonic == 'jmp':
            continue
    assert position + 1 < len(action)
    pending.append(action[position + 1].address)
assert found_return, 'Failure does not return through normal cleanup'

next_point = call_index(start, '?GetNextWayPoint@SmartAI@@')
move = call_index(start, '?MovePoint@MotionMaster@@')
dispatch = call_index(start, '?ProcessEventsFor@SmartScript@@')
combat = call_index(start, '?isInCombat@Unit@@')
assert combat < next_point < move < dispatch, 'Native combat/first point/movement/event order changed'
assert native['symbol']('?GetPath@SmartWaypointMgr@@') in native['calls']('?StartPath@SmartAI@@'), 'Native path lookup is missing'
escort = [index for index, entry in enumerate(start) if entry.mnemonic == 'or' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x1a8 and entry.operands[1].type == CS_OP_IMM and entry.operands[1].imm == 1]
assert len(escort) == 1 and next_point < escort[0] < move, 'Escort is armed before first-point admission'
failure = [entry.address for entry in start if entry.mnemonic == 'xor' and entry.op_str == 'al, al']
assert len(failure) == 1, 'Reinspect false-return path'
success = [index for index, entry in enumerate(start) if entry.mnemonic == 'mov' and entry.op_str == 'al, 1']
assert len(success) == 1 and success[0] > dispatch, 'StartPath succeeds before native movement and waypoint event'
missing = start[next_point + 1:escort[0]]
assert any(entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x1ac for entry in missing), 'Failed first point does not reset its cursor'
assert any(entry.mnemonic == 'jmp' and direct_target(entry) == failure[0] for entry in missing), 'Failed first point does not return false'
print('PASS: canonical bool StartPath retains native combat/path/first-node admission before escort, MovePoint and waypoint-start; actual WP_START consumes success but resets rejected runOnce and returns without LINK dispatch')
