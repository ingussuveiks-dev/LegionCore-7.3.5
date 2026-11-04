"""Registered original Craggle offer: qualified per-player native aura receipt."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_smoke_release.py')))
n, ins = p['n'], p['ins']
scope = 'npc_kingslayers_small_eventsAI@npc_kingslayers_small_events@@'
body, update = [ins('?' + name + '@' + scope) for name in ('UpdateSmokeOffer', 'UpdateAI')]
pairs = lambda rows: {(op.mnemonic, op.op_str) for op in rows}
dispatch, = sites(n, update, '?UpdateSmokeOffer@' + scope)
assert {108177, 1620, 360801} <= values(update[:dispatch])
for exact in (('cmp', 'eax, 0x1a691'), ('cmp', 'dword ptr [rax + 0xe0], 0x654'),
              ('cmp', 'qword ptr [rax + 0x3a88], 0x58161')):
    at = max(i for i, op in enumerate(update[:dispatch]) if (op.mnemonic, op.op_str) == exact)
    assert update[at + 1].mnemonic == 'jne'
    assert update[dispatch].address not in reachable(update, update[at + 1].operands[0].imm)
assert {1000, 1123, 48533, 227061, 42504, 42627} <= values(body)
assert 15.0 in floats(n, body)
for name in ('?IsInWorld@Object@@', '?IsPreDelete@Object@@', '?IsQuestObjectExpired@WorldObject@@',
             '?CanContact@Player@@', '?CanUpdateCriteria@Scenario@@', '?GetCriteriaProgressMap@',
             '?GetPlayerListInGrid@WorldObject@@', '?InSamePhase@WorldObject@@',
             '?IsWithinDistInMap@WorldObject@@', '?CanTargetQuestObject@WorldObject@@',
             '?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@'):
    assert sites(n, body, name), name
grant, = sites(n, body, '?AddAura@Unit@@')
cast, = sites(n, body, '?CastSpell@Unit@@')
has = sites(n, body, '?HasAura@Unit@@')
assert len(has) == 2 and has[0] < grant < cast < has[1]
assert body[has[0] + 2].mnemonic == body[has[1] + 2].mnemonic == 'jne'
assert body[has[0] + 2].operands[0].imm == body[has[1] + 2].operands[0].imm
assert {('cmp', 'al, 4'), ('cmp', 'qword ptr [rcx + 0x28], 0'),
        ('mov', 'dword ptr [rcx + 0x3c], 0x3e8'),
        ('mov', 'r8, rdi'), ('mov', 'rcx, rdi')} <= pairs(body[:grant])
clock, = [i for i, op in enumerate(body[:grant]) if (op.mnemonic, op.op_str) == ('cmp', 'eax, edx')]
assert body[clock + 1].mnemonic == 'jbe'
assert body[grant].address not in reachable(body, body[clock + 2].address)
assert {('add', 'rdi, 0x5a'), ('mov', 'byte ptr [rdi], 1')} <= pairs(body[has[1]:])
for target in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@',
               '?CompleteQuest@Player@@', '?AddItem@Player@@', '?FindNearestPlayer@WorldObject@@'):
    assert not sites(n, body, target)
print('PASS registered Craggle108177/360801/map1620 smoke offer: original1s poll, eligible actual Rogue/quest/current48533 and per-player missing227061 before native self aura/cast; no actor one-shot, no existing-aura refresh, bounded warning/retry, no progress/item shortcut. Actual client extra-action UI and full chains remain separate.')
