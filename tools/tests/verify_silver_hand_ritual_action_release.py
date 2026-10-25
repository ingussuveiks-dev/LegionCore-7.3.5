"""Verify third Travard's selected stage8 action admission and delayed recheck."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_final_dispatch_release.py')))
native = previous['native']
instructions = previous['previous']['instructions']


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


helper = '?CanAdvance@SilverHandRitualEntry@@'
gate = instructions(helper)
deny = next(entry.address for entry in gate if entry.mnemonic == 'xor' and entry.op_str == 'al, al')
for operands, branch in (('rcx, rcx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('dword ptr [rbx + 0xe0], 0x603', 'jne'), ('qword ptr [rbx + 8], 0', 'je'),
                         ('rax, rax', 'je'), ('eax, 0x444', 'jne'), ('eax, 8', 'jne')):
    position = next(index for index, entry in enumerate(gate) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operands)
    assert gate[position + 1].mnemonic == branch and gate[position + 1].operands[0].imm == deny
for method, branch in (('?IsInWorld@Object@@', 'je'), ('?IsPreDelete@Object@@', 'jne')):
    position, = sites(gate, method)
    assert gate[position + 1].op_str == 'al, al'
    assert gate[position + 2].mnemonic == branch and gate[position + 2].operands[0].imm == deny
entry, = sites(gate, '?GetUInt32Value@Object@@')
assert gate[entry - 2].op_str == 'edx, 9' and gate[entry + 1].op_str == 'eax, 0x1a0b4'
assert gate[entry + 2].mnemonic == 'jne' and gate[entry + 2].operands[0].imm == deny
scenario, = sites(gate, '?GetScenario@ScenarioMgr@@')
assert any(entry.op_str == 'edx, dword ptr [rbx + 0x3b8]' for entry in gate[scenario - 3:scenario])
assert sites(gate, '?GetScenarioId@Scenario@@') and sites(gate, '?GetCurrentStep@Scenario@@')
action_name = '?DoAction@npc_travar_thirdAI@'
action = instructions(action_name)
assert action[0].op_str == 'edx, 1' and action[0].mnemonic == 'cmp' and action[1].mnemonic == 'jne'
assert action[-1].mnemonic == 'ret' and action[1].operands[0].imm == action[-1].address
pending = next(index for index, entry in enumerate(action) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rcx + 0x169], 0')
valid, = sites(action, helper)
clock, = sites(action, '?CalculateTime@FunctionProcessor@@')
queue, = sites(action, '?AddFunction@FunctionProcessor@@')
assert pending < valid < clock < queue
exit_address = action[pending + 1].operands[0].imm
assert action[pending + 1].mnemonic == 'jne' and action[valid + 2].mnemonic == 'je'
assert action[valid + 2].operands[0].imm == exit_address and exit_address > action[queue].address
assert action[valid + 3].op_str == 'byte ptr [rdi + 0x169], 1'
assert action[clock - 2].op_str == 'edx, 0x3e8' and action[clock - 1].op_str == 'rcx, [rdi + 0x60]'
assert action[queue - 1].op_str == 'rcx, [rdi + 0x60]'
assert any(entry.op_str == 'qword ptr [rsp + 0x28], rdi' for entry in action[clock:queue])
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'DoAction@npc_travar_thirdAI@' in name]
tables = []
for index, entry in enumerate(action[:queue]):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            assert action[index + 1].op_str == 'qword ptr [rsp + 0x20], rcx'
            tables.append(address)
assert len(tables) == 1
delayed = instructions(closure)
valid, = sites(delayed, helper)
talk, = sites(delayed, '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')
move, = sites(delayed, '?MovePoint@MotionMaster@@')
assert valid < talk < move and delayed[valid + 2].mnemonic == 'jne'
assert [entry.op_str for entry in delayed[valid - 2:valid]] == ['rcx, qword ptr [rcx + 8]', 'rcx, qword ptr [rcx + 0x20]']
assert any(entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rax + 0x169], 0' for entry in delayed[valid:talk])
assert any(entry.mnemonic == 'ret' for entry in delayed[valid:talk])
assert any(entry.op_str == 'dl, 8' for entry in delayed[valid:talk])
factory = instructions('?GetAI@npc_travar_third@@')
assert any(entry.op_str == 'word ptr [rsi + 0x168], bp' for entry in factory)
assert any(entry.mnemonic == 'xor' and entry.op_str == 'ebp, ebp' for entry in factory)
assert any(entry.op_str == 'ecx, 0x170' for entry in factory)
print('PASS: selected third DoAction0x48 rejects non1/started, requires actual live nonpredeleted106676/map1539 and instance scenario1092/exact8, latchesAI+0x169 before native own queue+0x60/time1000. Actual selected closure rechecks same live gate before original Talk8/MovePoint1, reopening latch only on invalid context; factory initializes entry/action flags together with proven zero. Reset/re-entry epochs, per-motion credits and client traversal remain separate.')
