"""Verify actual last Travard LOS admission before ritual effects and wave queue."""
from pathlib import Path
import runpy
import struct

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_wave_lifetime_release.py')))
native, instructions, sites = previous['native'], previous['instructions'], previous['sites']
helper = '?CanStart@SilverHandFinalEntry@@'
policy = instructions(helper)
deny = next(entry.address for index, entry in enumerate(policy[:-1]) if entry.op_str == 'rbx, qword ptr [rsp + 0x40]'
            and policy[index + 1].mnemonic == 'xor' and policy[index + 1].op_str == 'al, al')
for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x4ff0], 1', 'jne'), ('byte ptr [rbx + 0x6848], 0', 'jne'),
                         ('byte ptr [rbx + 0x6849], 0', 'jne'), ('al, 2', 'jne'),
                         ('dword ptr [rdi + 0xa18], 0', 'jne'), ('eax, 0x1a119', 'jne'),
                         ('dword ptr [rdi + 0xe0], 0x603', 'jne'), ('rax, rax', 'je'),
                         ('rax, qword ptr [rdi + 8]', 'jne'), ('dword ptr [rbx + 0x3b8], eax', 'jne'),
                         ('eax, 3', 'jne'), ('eax, 0x444', 'jne'), ('eax, 9', 'jne')):
    positions = [index for index, entry in enumerate(policy) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operands]
    assert positions
    for position in positions:
        assert policy[position + 1].mnemonic == branch and policy[position + 1].operands[0].imm == deny
for method, branch, count in (('?IsInWorld@Object@@', 'je', 2), ('?IsPreDelete@Object@@', 'jne', 2),
                             ('?CanContact@Player@@', 'je', 1), ('?InSamePhase@WorldObject@@', 'je', 1),
                             ('?canSeeOrDetect@WorldObject@@', 'je', 1), ('?IsWithinLOSInMap@WorldObject@@', 'je', 1),
                             ('?IsWithinDistInMap@WorldObject@@', 'je', 1), ('?GetQuestRewardStatus@Player@@', 'jne', 1)):
    calls = sites(policy, method)
    assert len(calls) == count
    for position in calls:
        assert policy[position + 1].op_str == 'al, al'
        assert policy[position + 2].mnemonic == branch and policy[position + 2].operands[0].imm == deny
for method in ('?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@'):
    position, = sites(policy, method)
    assert policy[position - 2].op_str == 'edx, 0xa488' and policy[position - 1].op_str == 'rcx, rbx'
identity, = sites(policy, '?GetUInt32Value@Object@@')
assert policy[identity - 2].op_str == 'edx, 9' and policy[identity - 1].op_str == 'rcx, rdi'
radius, = [entry for entry in policy if entry.mnemonic == 'movss' and entry.op_str.startswith('xmm2, dword ptr [rip')]
assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 100.0
scenario, = sites(policy, '?GetScenario@ScenarioMgr@@')
assert policy[scenario - 2].op_str == 'edx, dword ptr [rdi + 0x3b8]'
assert sites(policy, '?GetScenarioId@Scenario@@') and sites(policy, '?GetCurrentStep@Scenario@@')
los = previous['los']
gate, = sites(los, helper)
latched = next(index for index, entry in enumerate(los) if entry.op_str == 'word ptr [rsi + 0x38], 0x101')
cast, = sites(los, '?DoCast@UnitAI@@QEAAXI@Z')
talk, = sites(los, '?Talk@CreatureAI@@QEAAXEVObjectGuid@@@Z')
assert gate < latched < cast < talk < previous['arm']
assert los[gate + 1].op_str == 'al, al' and los[gate + 2].mnemonic == 'je'
exit_address = los[gate + 2].operands[0].imm
pending = next(index for index, entry in enumerate(los[:gate]) if entry.op_str == 'byte ptr [rcx + 0x38], 0')
assert los[pending + 1].mnemonic == 'jne' and los[pending + 1].operands[0].imm == exit_address
assert any(entry.op_str == 'edi, edi' and entry.mnemonic == 'xor' for entry in los[:gate])
assert any(entry.op_str == 'rdx, rdx' and entry.mnemonic == 'test' for entry in los[:gate])
assert any(entry.op_str == 'rax, rdi' and entry.mnemonic == 'cmovne' for entry in los[:gate])
assert [entry.op_str for entry in los[gate - 2:gate]] == ['rdx, qword ptr [rcx + 0x20]', 'rcx, rax']
assert los[cast - 2].op_str == 'edx, 0x348da' and los[talk - 2].op_str == 'edx, edx'
for method in ('?FindNearestPlayer@WorldObject@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@'):
    assert not sites(los, method) and not sites(policy, method)
print('PASS: selected last LOS0x270 rejects initialized intro before null-safe actual Player conversion; actual CanStart requires live nonpredeleted Player/106777, active nonrewarded42120 Paladin, nonGM/nonteleport, same actual1539 map/instance/phase/visibility/LOS/100yd and instance1092/exact9. Both intro/criteria flags latch before original215258/Talk0/lights/own5000ms queue. No nearest substitute or earlier-stage50915 in LOS. Accepted-wave callbacks, periodic51838/combat completion and client VFX remain separate.')
