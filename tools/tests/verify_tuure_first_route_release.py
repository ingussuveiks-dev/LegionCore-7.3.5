"""Follow the selected Tuure real second kill and native waypoint arrival route."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM, CS_OP_REG, CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_first_fight_release.py')))
n, instructions, values = p['n'], p['instructions'], p['values']
scope = 'instance_tuure_InstanceMapScript@instance_tuure@@'
record, update, progress = ('?' + name + '@' + scope for name in ('RecordRouteDeath', 'UpdateRoute', 'HasRouteProgress'))
assert n['symbol'](record) in n['calls']('?CreatureDies@' + scope)
assert n['symbol'](update) in n['calls']('?Update@' + scope)
assert {106221, 361429, 1085, 41993} <= values(record)
assert {9859002, 47883, 47882, 50831, 106221, 1000, 5000, 2007, 2039, 361432} <= values(update)
for prefix in ('?Recipient@TuureFirstFight@@', '?AddQuestDelayedEvent@Player@@', '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@'):
    assert n['symbol'](prefix) in n['calls'](record), prefix
native_credit = '?UpdateAchievementCriteria@Scenario@@QEAAXW4CriteriaTypes'
for prefix in (native_credit, '?UpdateAchievementCriteria@Player@@', '?MovePath@MotionMaster@@QEAAXI_N',
               '?GetMotionSlot@MotionMaster@@', '?Eligible@TuureFirstFight@@', '?SendChat@CreatureTextMgr@@'):
    assert n['symbol'](prefix) in n['calls'](update), prefix
assert n['symbol'](progress) in n['calls'](update)
for prefix in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?RewardPlayerAndGroupAtKill@Player@@',
               '?CompleteQuest@Player@@', '?SetHealth@Unit@@', '?TeleportTo@Player@@', '?NearTeleportTo@'):
    assert not {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith(prefix)} & n['calls'](update), prefix
# Native scenario kill credit is separate from the50ms personal quest callback.
assert n['symbol'](native_credit) in n['calls']('?Reward@KillRewarder@@')
body = instructions('?Kill@Unit@@')
reward = n['symbol']('?RewardPlayerAndGroupAtKill@Player@@')
rewards = [i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) == reward]
slots = [i for i, x in enumerate(body) if x.mnemonic == 'mov' and len(x.operands) == 2 and x.operands[0].type == CS_OP_REG and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.disp == 0x28 and any(y.mnemonic == 'call' and y.operands[0].type == CS_OP_REG and y.operands[0].reg == x.operands[0].reg for y in body[i+1:i+5])]
assert rewards and len(slots) == 1 and max(rewards) < slots[0]
actor = 'npc_vindicator_borosAI@npc_vindicator_boros@@'
legacy_action = '?DoAction@' + actor
if any(name.startswith(legacy_action) for name in n['symbols']):
    assert not {9859002, 50831} & values(legacy_action)
else:
    # The later captain repair retires action2 as well. The selected Boros
    # factory must then inherit the native empty action, not an unused helper.
    refs = p['references']('?GetAI@npc_vindicator_boros@@')
    empty = n['symbol']('?DoAction@UnitAI@@UEAAXH@Z')
    selected = [struct.unpack_from('<80Q', n['binary'], n['offset'](a)) for a in refs]
    selected = [table for table in selected if table[0xa8//8] == n['symbol']('?DamageTakenAfterScaling@'+actor)]
    assert len(selected) == 1 and selected[0][0x48//8] == empty
assert 50831 not in values('?MovementInform@' + actor)
assert n['symbol']('?UpdateAchievementCriteria@Player@@') not in n['calls']('?MovementInform@' + actor)
# Follow the real arrival write and the matching field read in the controller.
arrive = '?OnArrived@?$WaypointMovementGenerator@VCreature@@@@'
arrival = instructions(arrive)
# MSVC inlines MovementInform; the null-AI branch still reaches the actual
# current-node -> Creature arrival write after both optional AI notifications.
stores = [(i, x) for i, x in enumerate(arrival) if x.mnemonic == 'mov' and len(x.operands) == 2 and
          x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp > 0x3000 and x.operands[0].size == 4 and
          x.operands[1].type == CS_OP_REG and i and arrival[i-1].mnemonic == 'mov' and
          arrival[i-1].operands[0].type == CS_OP_REG and arrival[i-1].operands[0].reg == x.operands[1].reg and
          arrival[i-1].operands[1].type == CS_OP_MEM and arrival[i-1].operands[1].mem.disp == 0x18]
assert len(stores) == 1
site, store = stores[0]
assert any(x.mnemonic == 'je' and x.operands[0].type == CS_OP_IMM and x.operands[0].imm == arrival[site-1].address for x in arrival[:site])
field = store.operands[0].mem.disp
route = instructions(update)
assert any(x.mnemonic == 'mov' and len(x.operands) == 2 and x.operands[0].type == CS_OP_REG and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.disp == field for x in route)
assert any(x.mnemonic == 'mov' and len(x.operands) == 2 and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == field and x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 0xffffffff for x in route)
assert any(x.mnemonic == 'cmp' and any(op.type == CS_OP_IMM and op.imm == 9859002 for op in x.operands) for x in instructions(update))
print('PASS actual selected second-kill/route helpers: native reward->Scenario before real CreatureDies, exact death/owner/lease, actual native47883/47882 with scoped Scenario-only allied fallback, native path9859002 and original indexes6/10; no old AI action1/nearest-player waypoint credit or direct quest/item/health/teleport grant. Native OnArrived dispatch retained; full extracted arrival/instance host owns reset/missing-AI/retry cases. Client route pending.')
