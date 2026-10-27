"""Follow the selected real captain death and native second-route continuation."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_cage_release.py')))
n, instructions, values, references = (p[k] for k in ('n', 'instructions', 'values', 'references'))
scope = 'instance_tuure_InstanceMapScript@instance_tuure@@'
m = {name: '?' + name + '@' + scope for name in (
    'EnterCombatForScript', 'RecordCaptainDeath', 'DeliverCaptainVictory', 'UpdateCaptain',
    'CaptainRecipient', 'BindCaptain', 'CaptainOwnerReady', 'UpdateEarnedCageActors', 'CaptainActor')}
assert any(struct.unpack_from('<Q', n['binary'], n['offset'](a) + 11*8)[0] == n['symbol'](m['EnterCombatForScript'])
           for a in references('?GetInstanceScript@instance_tuure@@'))
assert n['symbol'](m['RecordCaptainDeath']) in n['calls']('?CreatureDies@' + scope)
assert n['symbol'](m['DeliverCaptainVictory']) in n['calls'](m['RecordCaptainDeath'])
assert n['symbol'](m['UpdateCaptain']) in n['calls']('?Update@' + scope)
assert n['symbol'](m['UpdateEarnedCageActors']) in n['calls']('?Update@' + scope)
assert {106107, 361436, 1085, 47561} <= values(m['RecordCaptainDeath'])
assert {1085, 47909, 47561, 41993, 50634, 1000, 5000} <= values(m['DeliverCaptainVictory'])
assert {1085, 47909, 41993, 9859003, 1000, 8000, 4000, 5000, 2015, 106318, 361888} <= values(m['UpdateCaptain'])
assert {106134, 106137, 106253} <= values(m['CaptainRecipient'])
assert n['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@') in n['calls'](m['CaptainRecipient'])
assert n['symbol']('?Eligible@TuureFirstFight@@') in n['calls'](m['CaptainRecipient'])
assert n['symbol']('?AddQuestDelayedEvent@Player@@') in n['calls'](m['BindCaptain'])
assert n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](m['DeliverCaptainVictory'])
for prefix in ('?MovePoint@MotionMaster@@', '?MovePath@MotionMaster@@QEAAXI_N', '?GetMotionSlot@MotionMaster@@'):
    assert n['symbol'](prefix) in n['calls'](m['UpdateCaptain']), prefix
assert n['symbol']('?EnterCombatForScript@InstanceScript@@') in n['calls'](m['EnterCombatForScript'])
body = instructions('?SetInCombatState@Unit@@')
assert any(x.mnemonic == 'call' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0x58 for x in body) or any(
    x.mnemonic == 'mov' and len(x.operands) == 2 and x.operands[0].type == CS_OP_REG and
    x.operands[1].type == CS_OP_MEM and x.operands[1].mem.disp == 0x58 and
    any(y.mnemonic == 'call' and y.operands[0].type == CS_OP_REG and y.operands[0].reg == x.operands[0].reg for y in body[i+1:i+5])
    for i, x in enumerate(body)), 'Actual native engagement does not dispatch instance hook'
for method in m.values():
    for forbidden in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@',
                      '?SetHealth@Unit@@', '?TeleportTo@Player@@', '?NearTeleportTo@', '?CompleteScenario@'):
        assert not {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(forbidden)} & n['calls'](method)
assert not any(s.startswith('?JustDied@npc_capitan_naranotAI@') for s in n['symbols'])
print('PASS selected captain real-death/engagement hooks, native owner and quest lease, actual50634/47909, original1s/8s/4s native point3/path9859003/Lady handoff and earned cage actor recovery; no direct quest/item/health/teleport grant. Extracted native host covers strict endpoints/final-node removal/AI loss/retry; actual client pending.')
