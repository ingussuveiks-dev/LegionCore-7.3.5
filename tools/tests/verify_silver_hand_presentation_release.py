"""Verify the installed Silver Hand scene callback and existing-item admission."""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
handler = '?Complete@SilverHandPresentation@@'
decoded = instructions(handler)
targets = native['calls'](handler)
values = {operand.imm for entry in decoded for operand in entry.operands if operand.type == CS_OP_IMM}
assert {1267, 1618, 1539, 42120, 1092, 128823, 211842, 10, 11}.issubset(values), 'Missing native Silver Hand scope'
for dependency in ('?HasSceneStatus@Player@@', '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@',
                   '?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetItemByEntry@Player@@',
                   '?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z', '?CanEquipItem@Player@@', '?SwapItem@Player@@',
                   '?SendEquipError@Player@@', '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@'):
    assert native['symbol'](dependency) in targets, f'Missing native scene/equipment consumer: {dependency}'
for forbidden in ('?AddItem@Player@@', '?StoreNewItem@Player@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@'):
    assert native['symbol'](forbidden) not in targets, f'Scene bypasses the existing-item spell: {forbidden}'
assert native['symbol'](handler) in native['calls']('?OnTrigger@scene_silver_hand_pickup@@'), 'Scene wrapper bypasses the helper'
assert native['symbol']('??0SceneTriggerScript@@') in native['calls']('?AddSC_SilverHand@@'), 'Wrong script registry'
tables = []
for entry in instructions('?AddSC_SilverHand@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = entry.address + entry.size + operand.mem.disp
            if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 8)[0] == native['symbol']('?OnTrigger@scene_silver_hand_pickup@@'):
                tables.append(address)
assert len(tables) == 1, 'Registration does not select the installed scene callback'
positions = {entry.address: index for index, entry in enumerate(decoded)}


def reaches(start, destination):
    pending = [start]
    visited = set()
    while pending:
        address = pending.pop()
        if address == destination:
            return True
        if address in visited or address not in positions:
            continue
        visited.add(address)
        entry = decoded[positions[address]]
        if entry.mnemonic in ('ret', 'int3'):
            continue
        if entry.mnemonic.startswith('j'):
            if entry.operands[0].type == CS_OP_IMM:
                pending.append(entry.operands[0].imm)
            if entry.mnemonic == 'jmp':
                continue
        pending.append(entry.address + entry.size)
    return False


def calls_to(prefix):
    return [index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


cast, = calls_to('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@')
equip, = calls_to('?CanEquipItem@Player@@')
swap, = calls_to('?SwapItem@Player@@')
error, = calls_to('?SendEquipError@Player@@')
owned, = calls_to('?GetItemByEntry@Player@@')
mainhands = calls_to('?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z')
assert len(mainhands) == 2 and owned < mainhands[0] < equip < error < swap < mainhands[1] < cast, 'Equipment admission order changed'
for position in mainhands:
    assert any(entry.op_str == 'r8b, 0xf' for entry in decoded[position - 4:position]), 'Not the physical mainhand slot'
    assert decoded[position + 1].op_str == 'rax, rsi' and decoded[position + 1].mnemonic == 'cmp', 'Not the existing item identity'
    branch = decoded[position + 2]
    assert branch.mnemonic == 'je' and reaches(branch.operands[0].imm, decoded[cast].address), 'Equipped identity does not authorize the spell'
failure = decoded[mainhands[1] + 3].address
assert not reaches(failure, decoded[cast].address), 'Failed native swap can still award spell credit'
assert not reaches(decoded[error + 1].address, decoded[cast].address), 'Rejected equipment can still award spell credit'
assert decoded[equip + 1].op_str == 'al, al' and decoded[equip + 2].mnemonic == 'je', 'Native equipment result ignored'
assert any(entry.mnemonic == 'test' and entry.op_str == 'eax, eax' for entry in decoded[cast + 1:cast + 5]), 'Native spell failure result ignored'
bounds = {}
for compare, branch in zip(decoded, decoded[1:]):
    if compare.mnemonic == 'cmp' and len(compare.operands) == 2 and compare.operands[1].type == CS_OP_IMM:
        expected = {10: 'jb', 11: 'ja'}.get(compare.operands[1].imm)
        if expected and branch.mnemonic == expected:
            bounds[compare.operands[1].imm] = branch.operands[0].imm
assert bounds.keys() == {10, 11} and bounds[10] == bounds[11], 'Wrong live scenario step range'
assert not reaches(bounds[10], decoded[cast].address), 'Out-of-stage rejection reaches equipment spell'
assert b'Silver Hand pickup: native swap did not equip existing 128823' in native['binary'], 'Failed swap diagnostic missing'
assert b'Silver Hand pickup: native 211842 failed with result' in native['binary'], 'Failed cast diagnostic missing'
for dependency in ('?GetItemByEntry@Player@@', '?CanEquipItem@Player@@', '?CanEquipChildItem@Player@@', '?SwapItem@Player@@', '?SendEquipError@Player@@'):
    assert native['symbol'](dependency) in native['calls']('?EffectForceEquipItem@Spell@@'), f'Native effect249 consumer missing: {dependency}'
assert native['symbol']('?TrigerScene@Player@@') in native['calls']('?SceneCompleted@Player@@'), 'Native completion bridge missing'
assert native['symbol']('?OnSceneTrigger@ScriptMgr@@') in native['calls']('?TrigerScene@Player@@'), 'Native scene dispatch missing'
dispatch = re.search(r'\s\?SpellEffects@@\S+\s+([0-9a-fA-F]{16})\s', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert dispatch, 'Native effect table missing'
table = native['offset'](int(dispatch.group(1), 16))
for effect, callback in ((61, '?EffectSendEvent@Spell@@'), (64, '?EffectTriggerSpell@Spell@@'),
                         (81, '?EffectNULL@Spell@@'), (90, '?EffectKillCreditPersonal@Spell@@'),
                         (134, '?EffectKillCredit@Spell@@'), (249, '?EffectForceEquipItem@Spell@@')):
    assert native['resolve'](struct.unpack_from('<Q', native['binary'], table + effect * 8)[0]) == native['symbol'](callback), f'Wrong actual effect{effect} dispatcher'
personal = native['calls']('?EffectKillCreditPersonal@Spell@@')
assert native['symbol']('?KilledMonsterCredit@Player@@') in personal, 'Native personal credit missing'
assert native['symbol']('?RewardPlayerAndGroupAtEvent@Player@@') not in personal, 'Effect90 unexpectedly broadcasts group credit'
assert native['symbol']('?UpdateAchievementCriteria@Player@@') in native['calls']('?EffectSendEvent@Spell@@'), 'Native script event credit missing'
print('PASS: installed1267/1618 registration requires live Holy1092/10-11 and carried128823; actual mainhand identity, native CanEquipItem/SwapItem/errors precede211842. Failed swap/equip cannot reach the spell. Actual table selects61 event/249 equip/90 PERSONAL credit/64 trigger;225877 effect81 remains native NULL. No direct item/credit grant. Not full spell/group/client execution.')
