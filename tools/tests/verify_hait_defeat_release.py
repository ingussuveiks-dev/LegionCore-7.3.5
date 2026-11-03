"""Actual registered first Demon death, native receipt and Hait continuation."""
from pathlib import Path
import hashlib
import json
import runpy
from capstone import CS_OP_IMM
from monk_intro_instance_pe import reachable
from sharasdal_erunak_entry_pe import sites, values, fields, floats

p = runpy.run_path(str(Path(__file__).with_name('verify_hait_opening_release.py')))
n, ins, table, resolved = (p[k] for k in ('n', 'ins', 'table', 'resolved'))
state = lambda name: '?' + name + '@State@MonkHaitOpening@@'
inst = lambda name: '?' + name + '@instance_monk_intro@@'

def needs(body, *names):
    for name in names:
        assert sites(n, body, name), ('Defeat native dependency', name)

assert n['monk_hait_defeat_successor'] and n['monk_hait_state_size'] == 152
_, raw = table('??_7instance_monk_intro@@6B@', 140)
death = ins(inst('CreatureDies'))
assert n['symbol'](inst('CreatureDies')) in resolved(raw)
assert 0x498 in fields(death)
# Death is inlined into the actual registered native callback. The original
# full Demon GUID, map identity, spawn, corpse/world flags and launch/lease
# guards precede the continuation; every retained death consumer still runs.
needs(death, state('Progress'), state('Retained'), '?GetGuidValue@Object@@',
      '??9ObjectGuid@@', '?IsInWorld@Object@@', '?IsPreDelete@Object@@',
      '?GetCreature@Map@@', '?HasFlag@Object@@', '?getMSTime@@', state('UpdateDefeat'))
assert {98011, 362135, 0x380} <= values(death)
assert {0x5d, 0x5e, 0x5f, 0x89, 0x8d, 0x68, 0x70} <= fields(death)
continuation, = sites(n, death, state('UpdateDefeat'))
nine, = sites(n, death, '?Death@State@MonkNineRescue@@')
chen, = sites(n, death, '?Death@State@MonkChenRescue@@')
jorvinax, = sites(n, death, '?Death@State@MonkJorvinax@@')
assert continuation < nine < chen < jorvinax
update = ins(state('Update'))
assert sites(n, update, state('UpdateDefeat')) and 0x89 in fields(update)
valid = ins(state('DefeatValid'))
needs(valid, state('Retained'), state('Victory'), '?GetScenario@ScenarioMgr@@',
      '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@')
assert {1014, 943} <= values(valid) and {0x68, 0x8a} <= fields(valid)
victory = ins(state('Victory'))
assert 29068 in values(victory)
contact = ins(state('Contact'))
needs(contact, state('Retained'), '?IsInWorld@Object@@', '?CanContact@Player@@',
      '?InSamePhase@WorldObject@@', '?ShouldHideFor@WorldObject@@', '?IsWithinDistInMap@WorldObject@@')
outro = ins(state('UpdateDefeat'))
revalidate = '??R<lambda_1>@?1??UpdateDefeat@State@MonkHaitOpening@@'
needs(ins(revalidate), state('DefeatValid'), '?GetCreature@Map@@')
needs(outro, revalidate, state('Victory'), state('Contact'),
      '?UpdateAchievementCriteria@Player@@', '?SendChat@CreatureTextMgr@@',
      '?MovePoint@MotionMaster@@', '?GetMotionSlot@MotionMaster@@', '?IsWithinDist3d@WorldObject@@',
      '?SetHomePosition@Creature@@', '?getMSTime@@', '?outMessage@Log@@')
op = {(o.mnemonic, o.op_str) for o in outro}
# Native MoveSpline GetId/Finalized inline to m_Id64 and done68/20 on the
# actual Unit8c8 spline. Keep installation identity, ID and pose independent.
assert {('cmp', 'rax, qword ptr [rsi + 0x60]'),
        ('mov', 'ecx, dword ptr [rax + 0x64]'),
        ('cmp', 'ecx, dword ptr [rsi + 0x80]'),
        ('mov', 'dword ptr [rsi + 0x84], ecx'),
        ('cmp', 'dword ptr [rcx + 0x64], eax'),
        ('test', 'byte ptr [rcx + 0x68], 0x20')} <= op
assert {46868, 92, 2000, 4000, 7000, 10000, 3000, 6000, 120000} <= values(outro)
assert {3805.70, 1833.99, 949.62, 0.5, 40.0, 100.0} <= floats(n, outro)
assert {0x60, 0x70, 0x74, 0x78, 0x7c, 0x80, 0x84, 0x88, 0x8a, 0x8b, 0x8c, 0x8d} <= fields(outro)
assert len(sites(n, outro, '?UpdateAchievementCriteria@Player@@')) == 1
for body in (death, valid, victory, contact, outro):
    for forbidden in ('?FindNearestPlayer@', '?AddItem@', '?CompleteQuest@', '?TeleportTo@', '?KilledMonsterCredit@'):
        assert not sites(n, body, forbidden), forbidden

# The former raw SetData and point callback payloads are retained for other
# spawns. A matching first actor cannot reach their progress/text/AI queues.
data = ins('?SetData@npc_master_haitAI@npc_master_hait@@')
guard = next(i for i, o in enumerate(data) if o.mnemonic == 'cmp' and o.op_str.endswith(' + 0x3a88], 0x58679'))
branch = data[guard+1]
assert branch.mnemonic == 'je' and {97778, 1014} <= values(data[:guard])
excluded = reachable(data, branch.operands[0].imm)
for target in ('?UpdateAchievementCriteria@Player@@', '?Talk@CreatureAI@@', '?AddDelayedEvent@FunctionProcessor@@'):
    assert all(data[i].address not in excluded for i in sites(n, data, target))
move = ins('?MovementInform@npc_master_haitAI@npc_master_hait@@')
guard = next(i for i, o in enumerate(move) if (o.mnemonic, o.op_str) == ('cmp', 'ebx, 1'))
assert {97778, 1014, 362105, 8} <= values(move[:guard])
branch = move[guard+1]
assert branch.mnemonic == 'je' and branch.operands[0].type == CS_OP_IMM
excluded = reachable(move, branch.operands[0].imm)
for target in ('?SetHomePosition@Creature@@', '?Talk@CreatureAI@@', '?AddDelayedEvent@FunctionProcessor@@'):
    assert all(move[i].address not in excluded for i in sites(n, move, target))
print('PASS registered4a0 instance/498-owned152-byte State: real full-GUID98011/362135 corpse after launch, native death dispatch and retained consumers; original weak accepted owner/contact, genuine46868/29068 acknowledgment/once/retry; native2/4/7/10 and arrival3/6 clocks, actual issued point/spline/pose, finite interrupted/install retry, original-speaker text and first-actor legacy exclusions. No inferred client/full943 completion.')
print(json.dumps({'world_sha256': hashlib.sha256(n['binary']).hexdigest(),
                  'instance_size': 0x4a0, 'state_size': 152, 'criterion': 29068, 'event': 46868}))
