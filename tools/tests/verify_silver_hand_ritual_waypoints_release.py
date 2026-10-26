"""Verify third Travard's actual waypoint recipients and per-AI credit latches."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_ritual_arrival_release.py')))
native, instructions, sites = previous['native'], previous['instructions'], previous['sites']
helper = '?SelectPlayer@SilverHandRitualWaypoint@@'
policy = instructions(helper)
deny = next(policy[index - 1].address for index, entry in enumerate(policy)
            if entry.op_str == 'rbx, qword ptr [rsp + 0x50]' and entry.mnemonic == 'mov')
assert next(entry for entry in policy if entry.address == deny).op_str == 'eax, eax'
scenario, = sites(policy, '?GetCurrentStep@Scenario@@')
for operands, branch in (('rcx, rcx', 'je'), ('dword ptr [rsi + 0xa18], 0', 'jne'),
                         ('eax, 0x1a0b4', 'jne'), ('dword ptr [rsi + 0xe0], 0x603', 'jne'),
                         ('qword ptr [rsi + 8], 0', 'je'), ('eax, 1', 'ja'),
                         ('rax, rax', 'je'), ('eax, 0x444', 'jne'), ('eax, edi', 'jne')):
    position, = [index for index, entry in enumerate(policy[:scenario + 3])
                 if entry.mnemonic in ('test', 'cmp') and entry.op_str == operands]
    assert policy[position + 1].mnemonic == branch and policy[position + 1].operands[0].imm == deny
assert any(entry.mnemonic == 'lea' and entry.op_str == 'eax, [rdi - 5]' for entry in policy[:scenario])
identity, = sites(policy, '?GetUInt32Value@Object@@')
assert policy[identity - 2].op_str == 'edx, 9' and policy[identity - 1].op_str == 'rcx, rsi'
instance, = sites(policy, '?GetScenario@ScenarioMgr@@')
assert policy[instance - 2].op_str == 'edx, dword ptr [rsi + 0x3b8]'
assert any(entry.op_str == 'rcx, qword ptr [rax + 0x768]' for entry in policy[scenario:])
reference = next(index for index, entry in enumerate(policy) if entry.op_str == 'rdi, qword ptr [rbx + 0x20]')
skip = next(entry.address for entry in policy if entry.op_str == 'rbx, qword ptr [rbx + 8]')
for method, branch in (('?IsInWorld@Object@@', 'je'), ('?IsPreDelete@Object@@', 'jne')):
    calls = sites(policy, method)
    assert len(calls) == 2 and calls[0] < reference < calls[1]
    for position, target in ((calls[0], deny), (calls[1], skip)):
        assert policy[position + 1].op_str == 'al, al'
        assert policy[position + 2].mnemonic == branch and policy[position + 2].operands[0].imm == target
for method, branch in (('?CanContact@Player@@', 'je'), ('?InSamePhase@WorldObject@@', 'je'),
                       ('?IsWithinDistInMap@WorldObject@@', 'je'), ('?GetQuestRewardStatus@Player@@', 'jne')):
    position, = sites(policy, method)
    assert policy[position + 1].op_str == 'al, al'
    assert policy[position + 2].mnemonic == branch and policy[position + 2].operands[0].imm == skip
for operands in ('rdi, rdi', 'byte ptr [rdi + 0x4ff0], 1', 'byte ptr [rdi + 0x6848], 0',
                 'byte ptr [rdi + 0x6849], 0', 'al, 2', 'qword ptr [rdi + 8], rax',
                 'dword ptr [rdi + 0x3b8], eax'):
    position, = [index for index, entry in enumerate(policy[reference:], reference)
                 if entry.op_str == operands and entry.mnemonic in ('cmp', 'test')]
    assert policy[position + 1].mnemonic == ('je' if operands == 'rdi, rdi' else 'jne')
    assert policy[position + 1].operands[0].imm == skip
radius, = [entry for entry in policy if entry.mnemonic == 'movss' and entry.op_str.startswith('xmm6, dword ptr [rip')]
assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 100.0
for method in ('?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@'):
    position, = sites(policy, method)
    assert policy[position - 2].op_str == 'edx, 0xa488' and policy[position - 1].op_str == 'rcx, rdi'
status, = sites(policy, '?GetQuestStatus@Player@@')
assert policy[status + 1].op_str == 'eax, 3' and policy[status + 2].mnemonic == 'je'
accepted = policy[status + 2].operands[0].imm
assert next(entry for entry in policy if entry.address == accepted).op_str == 'rax, rdi'
assert any(entry.mnemonic == 'jne' and entry.operands[0].imm == policy[reference].address for entry in policy[status:])
assert not any(entry.op_str == 'dword ptr [rdi + 0xa18], 0' for entry in policy)

movement = previous['movement']
selectors = sites(movement, helper)
credits = sites(movement, '?UpdateAchievementCriteria@Player@@')
assert len(selectors) == len(credits) == 2
waypoint = next(index for index, entry in enumerate(movement) if entry.op_str == 'esi, 2' and entry.mnemonic == 'cmp')
point = next(index for index, entry in enumerate(movement) if entry.op_str == 'esi, 8' and entry.mnemonic == 'cmp')
assert movement[waypoint + 1].mnemonic == 'jne' and movement[waypoint + 1].operands[0].imm == movement[point].address
for node, step, event, offset, selection, credit in zip((9, 19), (5, 6), (50877, 50912), (0x16d, 0x16e), selectors, credits):
    beginning, = [index for index, entry in enumerate(movement[:selection]) if entry.op_str == f'edi, {hex(node) if node > 9 else node}' and entry.mnemonic == 'cmp']
    latch, = [index for index, entry in enumerate(movement[beginning:selection], beginning)
              if entry.op_str == f'byte ptr [rbx + {hex(offset)}], 0' and entry.mnemonic == 'cmp']
    assert movement[latch + 1].mnemonic == 'jne' and movement[latch + 1].operands[0].imm > movement[point].address
    assert movement[selection - 2].op_str == f'edx, {step}'
    assert movement[selection - 1].op_str == 'rcx, qword ptr [rbx + 0x20]'
    assert movement[selection + 1].op_str == 'rax, rax' and movement[selection + 2].mnemonic == 'je'
    assert movement[selection + 2].operands[0].imm == movement[latch + 1].operands[0].imm
    assert movement[selection + 3].op_str == f'byte ptr [rbx + {hex(offset)}], 1'
    assert movement[credit - 3].op_str == 'edx, 0x5c' and movement[credit - 2].op_str == f'r8d, {hex(event)}'
    assert movement[credit - 1].op_str == 'rcx, rax' and waypoint < beginning < selection < credit < point
factory = previous['factory']
assert any(entry.op_str == 'word ptr [rsi + 0x16c], bp' for entry in factory)
assert any(entry.op_str == 'byte ptr [rsi + 0x16e], bpl' for entry in factory)
assert not any(operand.type == CS_OP_MEM and operand.mem.disp in (0x16d, 0x16e)
               for entry in instructions('?Reset@npc_travar_thirdAI@') for operand in entry.operands)
for method in ('?FindNearestPlayer@WorldObject@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@'):
    assert not sites(policy, method) and not sites(movement, method)
print('PASS: selected third MovementInform0x1a8 WAYPOINT2/id9 and19 call live106676/1539/1092 exact5/6 selector, choose one eligible actual-map active42120 Paladin within100yd, reject shared-map/ineligible nearest substitutes and latch initialized AI+16d/16e before one92/50877-or50912. Reset retains latches; POINT1/queues/arrival remain separately verified. No motion-request ownership, missing-callback retry, persisted epoch or client/VFX proof.')
