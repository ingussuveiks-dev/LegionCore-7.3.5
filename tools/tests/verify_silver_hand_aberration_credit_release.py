"""Verify the installed aberration death recipient, admission and one-shot latch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_ritual_entry_release.py')))
native, instructions = previous['native'], previous['instructions']
sites, reaches = previous['sites'], previous['reaches']
helper = '?CanCredit@SilverHandAberration@@'
body = instructions(helper)
deny = next(entry.address for index, entry in enumerate(body[:-1]) if entry.op_str == 'rbx, qword ptr [rsp + 0x40]'
            and body[index + 1].mnemonic == 'xor' and body[index + 1].op_str == 'al, al')


def reject(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == deny


for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'), ('byte ptr [rbx + 0x4ff0], 1', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('dword ptr [rdi + 0xa18], 0', 'je'), ('dword ptr [rdi + 0xe0], 0x603', 'jne'),
                         ('rax, rax', 'je'), ('rax, qword ptr [rdi + 8]', 'jne'),
                         ('dword ptr [rbx + 0x3b8], eax', 'jne')):
    position = next(index for index, entry in enumerate(body) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test'))
    reject(position, branch)
assert not any(entry.op_str == 'dword ptr [rbx + 0xa18], 0' for entry in body), 'Dead owner wrongly rejected'
for method in ('?IsInWorld@Object@@', '?CanContact@Player@@', '?InSamePhase@WorldObject@@', '?IsWithinDistInMap@WorldObject@@'):
    positions = sites(body, method)
    assert len(positions) == (2 if method == '?IsInWorld@Object@@' else 1)
    for position in positions:
        assert body[position + 1].op_str == 'al, al'
        reject(position + 1, 'je')
predelete = sites(body, '?IsPreDelete@Object@@')
assert len(predelete) == 2
for position in predelete:
    assert body[position + 1].op_str == 'al, al'
    reject(position + 1, 'jne')
for method, operands in (('?GetUInt32Value@Object@@', 'eax, 0x1a0ad'), ('?GetByteValue@Object@@', 'al, 2'),
                         ('?GetQuestStatus@Player@@', 'eax, 3'), ('?GetScenarioId@Scenario@@', 'eax, 0x444'),
                         ('?GetCurrentStep@Scenario@@', 'eax, 5')):
    position, = sites(body, method)
    assert body[position + 1].op_str == operands
    reject(position + 1, 'jne')
rewarded, = sites(body, '?GetQuestRewardStatus@Player@@')
assert body[rewarded + 1].op_str == 'al, al'
reject(rewarded + 1, 'jne')
quest, = sites(body, '?GetQuestStatus@Player@@')
assert all(any(entry.op_str == 'edx, 0xa488' for entry in body[position - 3:position]) for position in (rewarded, quest))
scenario, = sites(body, '?GetScenario@ScenarioMgr@@')
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
distance, = sites(body, '?IsWithinDistInMap@WorldObject@@')
assert previous['floats'](body[distance - 7:distance]) == [100.0]

callback = '?JustDied@npc_horrific_aberrationAI@npc_horrific_aberration@@'
body = instructions(callback)
credit, = sites(body, '?UpdateAchievementCriteria@Player@@')
owner, = sites(body, '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
gates = sites(body, helper)
assert len(gates) == 2 and owner < gates[0] < gates[1] < credit
assert body[owner + 1].op_str == 'rdx, qword ptr [rdi + 0x20]'
assert body[owner + 2].op_str == 'rcx, rax' and body[owner + 3].op_str == 'rbx, rax'
assert body[gates[0] + 1].op_str == 'esi, esi' and body[gates[0] + 1].mnemonic == 'xor'
assert body[gates[0] + 2].op_str == 'al, al' and body[gates[0] + 3].mnemonic == 'jne'
selected = next(index for index, entry in enumerate(body) if entry.mnemonic == 'test' and entry.op_str == 'rbx, rbx'
                and body[index - 1].op_str == 'rbx, qword ptr [rbx + 0x20]' and index > gates[1])
assert body[gates[0] + 3].operands[0].imm == body[selected].address
assert body[selected + 1].mnemonic == 'je' and not reaches(body, body[selected + 1].operands[0].imm, body[credit].address)
for operands, branch in (('byte ptr [rcx + 0x59], 0', 'jne'), ('rdx, rdx', 'je'),
                         ('qword ptr [rax + 8], 0', 'je'), ('qword ptr [rbx + 8], rcx', 'jne')):
    position = next(index for index, entry in enumerate(body[:owner]) if entry.op_str == operands and entry.mnemonic in ('cmp', 'test'))
    jump = next(entry for entry in body[position + 1:position + 5] if entry.mnemonic == branch)
    assert not reaches(body, jump.operands[0].imm, body[credit].address)
world, = sites(body, '?IsInWorld@Object@@')
assert body[world - 1].op_str == 'rcx, rdx' and body[world + 1].op_str == 'al, al'
assert body[world + 2].mnemonic == 'je' and not reaches(body, body[world + 2].operands[0].imm, body[credit].address)
assert any(entry.op_str == 'rax, qword ptr [rcx + 0x768]' for entry in body[gates[0]:gates[1]])
for index in range(gates[0] + 4, gates[1]):
    if body[index].mnemonic == 'je':
        assert not reaches(body, body[index].operands[0].imm, body[credit].address)
assert body[gates[1] - 1].op_str == 'rcx, qword ptr [rbx + 0x20]'
assert body[gates[1] + 1].op_str == 'al, al' and body[gates[1] + 2].mnemonic == 'jne'
valid_member = next(index for index, entry in enumerate(body) if entry.op_str == 'rbx, qword ptr [rbx + 0x20]' and index > gates[1])
assert body[gates[1] + 2].operands[0].imm == body[valid_member].address
assert body[gates[1] + 3].op_str == 'rbx, qword ptr [rbx + 8]'
assert any(entry.mnemonic == 'jne' and entry.operands[0].imm == body[gates[1] - 2].address for entry in body[gates[1] + 3:valid_member])
for index in range(gates[1] + 3, valid_member):
    if body[index].mnemonic == 'je':
        assert not reaches(body, body[index].operands[0].imm, body[credit].address)
assert body[credit - 1].op_str == 'byte ptr [rdi + 0x59], 1'
for operands in ('edx, 0x5c', 'r8d, 0xc6d0', 'rcx, rbx'):
    assert any(entry.op_str == operands for entry in body[credit - 9:credit])
for method in ('?FindNearestPlayer@WorldObject@@', '?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?RewardPlayerAndGroupAtKill@Player@@'):
    assert not sites(body, method)

factory = instructions('?GetAI@npc_horrific_aberration@@')
tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if slots[0x130 // 8] == native['symbol'](callback):
            tables.append(slots)
assert len(tables) == 1
reset = '?Reset@npc_horrific_aberrationAI@npc_horrific_aberration@@'
assert tables[0][0x28 // 8] == native['symbol'](reset)
assert any(entry.op_str == 'byte ptr [rsi + 0x59], bpl' for entry in factory)
assert any(entry.op_str == 'word ptr [rbx + 0x58], 0' for entry in instructions(reset))
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x130
           for entry in instructions('?Kill@Unit@@'))
scenario_targets = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith('?UpdateAchievementCriteria@Scenario@@')}
assert scenario_targets & native['calls']('?UpdateAchievementCriteria@Player@@')
print('PASS: selected aberration death slot0x130 and Reset slot0x28 retain native owner/charmer resolution, full nonpredelete/dead-actor/live42120/1092/exact5/100yd gate and one eligible map-participant fallback for NPC/missing/ineligible-owner shared kills. Empty/all-ineligible lists cannot retain the rejected owner. AI+0x59 is initialized/reset and set before one92/50896 dispatch through native shared Scenario bridge. No alive-owner interaction gate, ineligible neighborhood recipient or manual party/item award. Half-health choreography, actual combat/group counters and client traversal remain separate.')
