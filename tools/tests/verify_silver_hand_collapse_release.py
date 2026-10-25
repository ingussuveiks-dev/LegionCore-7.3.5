"""Verify the registered Holy collapse bridge and native exit consumers."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_real_loot_release.py')))
native = previous['native']
instructions = previous['instructions']
reaches = previous['reaches']
sites = previous['sites']
helper = '?Complete@SilverHandCollapse@@'
body = instructions(helper)
cast, = sites(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@')


def reject(position, branch):
    assert body[position + 1].mnemonic == branch
    assert not reaches(body, body[position + 1].operands[0].imm, body[cast].address), 'Rejected collapse reaches the exit spell'


for operands, branch in (('rdx, rdx', 'je'), ('dword ptr [rdx + 4], 0x4f4', 'jne'),
                         ('dword ptr [rdx], 0x653', 'jne'), ('rax, 8', 'jne'),
                         ('qword ptr [r8], rax', 'jne'), ('rbx, rbx', 'je'),
                         ('dword ptr [rbx + 0xa18], r14d', 'jne'),
                         ('byte ptr [rbx + 0x6848], r14b', 'jne'), ('byte ptr [rbx + 0x6849], r14b', 'jne'),
                         ('qword ptr [rbx + 8], r14', 'je'), ('dword ptr [rbx + 0xe0], 0x603', 'jne')):
    position = next(index for index, entry in enumerate(body[:cast]) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operands)
    reject(position, branch)
assert any(entry.mnemonic == 'movabs' and entry.op_str == 'rax, 0x6574656c706d6f63' for entry in body[:cast])
assert any(entry.mnemonic == 'xor' and entry.op_str == 'r14d, r14d' for entry in body[:cast])
for prefix, branch in (('?HasAura@Unit@@', 'je'), ('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'),
                       ('?GetQuestRewardStatus@Player@@', 'jne'), ('?IsCompleted@Scenario@@', 'je')):
    position, = sites(body, prefix)
    assert body[position + 1].op_str == 'al, al'
    reject(position + 1, branch)
scene_checks = sites(body, '?HasSceneStatus@Player@@')
assert len(scene_checks) == 2
for position, scene in zip(scene_checks, (1268, 1267)):
    assert any(entry.op_str == f'edx, {scene:#x}' for entry in body[position - 5:position])
    assert any(entry.mnemonic == 'mov' and entry.op_str.endswith(', 3') for entry in body[position - 6:position])
    assert body[position + 1].op_str == 'al, al'
    reject(position + 1, 'je')
aura, = sites(body, '?HasAura@Unit@@')
assert any(entry.op_str == 'edx, 0x33b96' for entry in body[aura - 7:aura])
klass, = sites(body, '?GetByteValue@Object@@')
assert body[klass + 1].op_str == 'al, 2'
reject(klass + 1, 'jne')
statuses = sites(body, '?GetQuestStatus@Player@@')
assert len(statuses) == 2 and all(any(entry.op_str == 'edx, 0xa488' for entry in body[position - 3:position]) for position in statuses)
assert body[statuses[0] + 1].op_str == 'eax, 3' and body[statuses[0] + 2].mnemonic == 'je'
assert body[statuses[1] + 1].op_str == 'eax, 1'
reject(statuses[1] + 1, 'jne')
scenario, = sites(body, '?GetScenario@ScenarioMgr@@')
assert any(entry.op_str == 'edx, dword ptr [rbx + 0x3b8]' for entry in body[scenario - 4:scenario])
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
identity, = sites(body, '?GetScenarioId@Scenario@@')
assert body[identity + 1].op_str == 'eax, 0x444'
reject(identity + 1, 'jne')
completed, = sites(body, '?IsCompleted@Scenario@@')
assert any(entry.op_str == 'edx, edx' and entry.mnemonic == 'xor' for entry in body[completed - 3:completed])
owned, = sites(body, '?GetItemByEntry@Player@@')
assert any(entry.op_str == 'edx, 0x1f737' for entry in body[owned - 4:owned])
assert body[owned + 2].op_str == 'rax, rax'
reject(owned + 2, 'je')
equipped, = sites(body, '?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z')
assert any(entry.op_str == 'r8b, 0xf' for entry in body[equipped - 4:equipped])
assert body[equipped + 1].op_str == 'rax, rdi'
reject(equipped + 1, 'jne')
destination, = sites(body, '?GetSpellTargetPosition@SpellMgr@@')
assert any(entry.op_str == 'edx, 0x33b9b' for entry in body[destination - 3:destination])
assert body[destination + 1].op_str == 'rax, rax'
reject(destination + 1, 'je')
assert body[destination + 3].op_str == 'dword ptr [rax], 0x603'
reject(destination + 3, 'jne')
assert all(any(entry.op_str == value for entry in body[cast - 6:cast]) for value in ('r8d, 0x33b9b', 'r9b, 1', 'rdx, rbx', 'rcx, rbx'))
assert any(entry.op_str == 'eax, eax' and entry.mnemonic == 'test' for entry in body[cast + 1:cast + 4])
targets = native['calls'](helper)
for forbidden in ('?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@', '?AddItem@Player@@', '?SwapItem@Player@@'):
    assert native['symbol'](forbidden) not in targets, f'Direct progression/teleport instead of native exit: {forbidden}'
teleports = {native['symbol'](name) for name in native['symbols'] if name.startswith('?TeleportTo@Player@@')}
assert teleports and not teleports.intersection(targets), 'Helper bypasses native exit with a direct teleport'
assert b'Silver Hand collapse: missing or foreign-map 211867 destination' in native['binary']
assert b'Silver Hand collapse: native exit 211867 failed with result' in native['binary']
assert native['symbol'](helper) in native['calls']('?OnTrigger@scene_silver_hand_collapse@@')
registered = []
for entry in instructions('?AddSC_SilverHand@@'):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 8)[0] == native['symbol']('?OnTrigger@scene_silver_hand_collapse@@'):
            registered.append(address)
assert len(registered) == 1, 'Registry does not select checked scene callback'
dispatch = re.search(r'\s\?SpellEffects@@\S+\s+([0-9a-fA-F]{16})\s', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert dispatch
table = native['offset'](int(dispatch.group(1), 16))
for effect, consumer in ((164, '?EffectRemoveAura@Spell@@'), (252, '?EffectTeleportUnits@Spell@@'), (61, '?EffectSendEvent@Spell@@')):
    assert native['resolve'](struct.unpack_from('<Q', native['binary'], table + effect * 8)[0]) == native['symbol'](consumer)
assert native['symbol']('?GetSpellTargetPosition@SpellMgr@@') in native['calls']('?SelectImplicitCasterDestTargets@Spell@@')
assert teleports.intersection(native['calls']('?EffectTeleportUnits@Spell@@')), 'Native effect252 no longer reaches a Player teleport'
assert native['symbol']('?AddDelayedEvent@FunctionProcessor@@') in native['calls']('?EffectRemoveAura@Spell@@')
assert native['symbol']('?UpdateAchievementCriteria@Player@@') in native['calls']('?EffectSendEvent@Spell@@')
print('PASS: registered1268/1619 actual-complete/latest-scene bridge gates live Holy1092 completion, prior1267, active211862, existing mainhand and real1539 destination before self-triggered211867. Actual native164/252/61 dispatch/DB target/teleport/event consumers retained; no manual105892/item/position grant. Cast acceptance is not actual arrival or client verification.')
