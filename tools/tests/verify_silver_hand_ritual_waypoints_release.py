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
completion_name = '?CompletePrisonWaypoints@npc_travar_thirdAI@'
completion = instructions(completion_name)
selectors = sites(completion, helper)
credits = sites(completion, '?UpdateAchievementCriteria@Player@@')
assert len(selectors) == len(credits) == 2
assert completion[3].mnemonic == 'xor' and completion[3].op_str == 'edi, edi'
for step, event, observed, credited, selection, credit, owner_register, target in zip(
        (5, 6), (50877, 50912), (0x16f, 0x170), (0x16d, 0x16e), selectors, credits,
        ('rcx', 'rbx'), (completion[credits[0] + 1].address, completion[-4].address)):
    observed_gate, = [index for index, entry in enumerate(completion[:selection])
                      if entry.mnemonic == 'cmp' and entry.op_str == f'byte ptr [{owner_register} + {hex(observed)}], dil']
    credited_gate, = [index for index, entry in enumerate(completion[:selection])
                      if entry.mnemonic == 'cmp' and entry.op_str == f'byte ptr [{owner_register} + {hex(credited)}], dil']
    assert completion[observed_gate + 1].mnemonic == 'je' and completion[observed_gate + 1].operands[0].imm == target
    assert completion[credited_gate + 1].mnemonic == 'jne' and completion[credited_gate + 1].operands[0].imm == target
    assert completion[selection - 1].op_str == f'edx, {step}'
    assert completion[selection - 2].op_str == f'rcx, qword ptr [{owner_register} + 0x20]'
    assert completion[selection + 1].op_str == 'rax, rax'
    assert completion[selection + 2].mnemonic == 'je' and completion[selection + 2].operands[0].imm == target
    assert completion[credit - 1].op_str == f'byte ptr [rbx + {hex(credited)}], 1'
    assert completion[credit - 2].op_str == 'rcx, rax'
    assert any(entry.op_str == 'edx, 0x5c' for entry in completion[selection:credit])
    assert any(entry.op_str == f'r8d, {hex(event)}' for entry in completion[selection:credit])
    assert observed_gate < credited_gate < selection < credit
waypoint = next(index for index, entry in enumerate(movement) if entry.op_str == 'esi, 2' and entry.mnemonic == 'cmp')
point = next(index for index, entry in enumerate(movement) if entry.op_str == 'esi, 8' and entry.mnemonic == 'cmp')
assert movement[waypoint + 1].mnemonic == 'jne' and movement[waypoint + 1].operands[0].imm == movement[point].address
attempts = sites(movement, completion_name)
assert len(attempts) == 3 and attempts[-1] < point
repeat = movement[attempts[-1] - 1].address
for node, speech, observed, credited, attempt in zip((9, 19), (5, 6), (0x16f, 0x170), (0x16d, 0x16e), attempts):
    beginning, = [index for index, entry in enumerate(movement[:attempt])
                   if entry.op_str == f'edi, {hex(node) if node > 9 else node}' and entry.mnemonic == 'cmp']
    remembered, = [index for index, entry in enumerate(movement[beginning:attempt], beginning)
                    if entry.mnemonic == 'mov' and entry.op_str == f'byte ptr [rbx + {hex(observed)}], 1']
    assert movement[remembered - 1].op_str == f'eax, byte ptr [rbx + {hex(observed)}]'
    assert movement[remembered + 1].op_str == 'al, al'
    assert movement[remembered + 2].mnemonic == 'jne' and movement[remembered + 2].operands[0].imm == repeat
    talks = sites(movement[remembered:attempt], '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')
    assert len(talks) == 1
    talk = remembered + talks[0]
    assert any(entry.op_str == f'dl, {speech}' for entry in movement[talk - 4:talk])
    assert movement[attempt - 1].op_str == 'rcx, rbx'
    assert movement[attempt + 1].op_str == f'byte ptr [rbx + {hex(credited)}], 0'
    assert movement[attempt + 2].mnemonic == 'jne'
    assert waypoint < beginning < remembered < talk < attempt < point
assert not sites(movement, '?UpdateAchievementCriteria@Player@@')
for node, stage in ((9, 5), (19, 6)):
    assert f'Silver Hand waypoint{node} has no eligible stage{stage} participant; retaining observed arrival for credit retry'.encode() in native['binary']
lifecycle = instructions('?UpdateLifecycle@npc_travar_thirdAI@')
recover, = sites(lifecycle, '?Recover@SilverHandCompanion@@')
retry, = sites(lifecycle, completion_name)
ritual, = sites(lifecycle, '?CompleteRitualArrival@npc_travar_thirdAI@')
assert recover < retry < ritual and lifecycle[retry - 1].op_str == 'rcx, rbx'
factory = previous['factory']
assert any(entry.op_str == 'qword ptr [rsi + 0x168], rbp' for entry in factory)
assert any(entry.op_str == 'byte ptr [rsi + 0x170], bpl' for entry in factory)
assert not any(operand.type == CS_OP_MEM and operand.mem.disp in (0x16d, 0x16e, 0x16f, 0x170)
               for entry in instructions('?Reset@npc_travar_thirdAI@') for operand in entry.operands)
for method in ('?FindNearestPlayer@WorldObject@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@',
               '?MovePath@MotionMaster@@', '?MovePoint@MotionMaster@@', '?DespawnOrUnsummon@Creature@@'):
    assert not sites(policy, method) and not sites(completion, method)
print('PASS: selected MovementInform0x1a8 remembers WAYPOINT2/id9/19 at AI+16f/170 before once-only Talk5/6, attempts completion and repeats without speech/log replay. Selected locked Lifecycle0x110 retries observed-only progress; actual completion requires arrived/uncredited, unchanged live106676/1539/1092 exact5/6 eligible-map42120/100yd selector, then latches16d/16e before one92/50877-or50912. Factory zeroes0x178 AI state and Reset preserves observations; no new timer/movement/despawn/item/party grant. No issued-path identity, never-delivered callback recovery, persisted epoch or client/VFX proof.')
