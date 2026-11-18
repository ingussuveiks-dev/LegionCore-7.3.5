"""Verify selected Ring registrations, native receipts and final-link consumers."""
from pathlib import Path
import runpy, struct
from capstone import CS_OP_IMM
p = runpy.run_path(str(Path(__file__).with_name('verify_gameobject_range_release.py')))
n, ins, refs = p['n'], p['ins'], p['p']['refs']
data = p['p']['data_symbols']
symbol = p['p']['data_symbol']
def values(name): return {o.imm for i in ins(name) for o in i.operands if o.type == CS_OP_IMM}
def require(caller, *targets):
 for target in targets: assert n['symbol'](target) in n['calls'](caller), (caller, target)
def slots(address):
 end = min(v for v in data.values() if v > address)
 assert 8 <= end-address <= 8192
 return tuple(n['resolve'](v) if v else 0 for v in struct.unpack_from('<'+'Q'*((end-address)//8), n['binary'], n['offset'](address)))
registration = '?AddSC_artifact_acquisition@@'
for script in ('npc_ring_reforged_nobundo', 'go_ring_reforged_rock'):
 table = symbol(f'??_7{script}@@6BScriptObject@@@')
 assert table in refs(registration)
 assert n['symbol'](f'?GetAI@{script}@@') in slots(table)
factory = '?GetAI@npc_ring_reforged_nobundo@@'
table = symbol('??_7AI@RingReforged@@6B@')
assert table in refs(factory)
for method in ('Reset','IsSummonedBy','UpdateAI','MovementInform','sGossipSelect','sQuestReward','CanBeVisibleFor'):
 assert n['symbol'](f'?{method}@AI@RingReforged@@') in slots(table)
table = symbol('??_7player_ring_reforged@@6B@')
assert n['symbol']('??0player_ring_reforged@@') in n['calls'](registration)
assert table in refs('??0player_ring_reforged@@')
for method in ('OnUpdate','OnMovementInform','OnBeforeLogout','OnLogout','OnMapChanged'):
 assert n['symbol'](f'?{method}@player_ring_reforged@@') in slots(table)
for script, base in [('spell_ring_reforged_assume','Spell'),('spell_ring_reforged_oath','Spell'),('spell_ring_reforged_rock','Spell'),('aura_ring_reforged_oath','Aura')]:
 loader = symbol(f'??_7?$Generic{base}ScriptLoader@V{script}@@@@6B@')
 assert loader in refs(registration)
 factory = f'?Get{base}Script@?$Generic{base}ScriptLoader@V{script}@@@@'
 assert n['symbol'](factory) in slots(loader)
 table = symbol(f'??_7{script}@@6B@')
 assert table in refs(factory)
 assert n['symbol'](f'?Register@{script}@@') in slots(table)
require('?Check@spell_ring_reforged_rock@@','?GetExplTargetGObj@SpellScript@@')
assert n['symbol']('?GetOriginalTargetGUID@SpellScript@@') not in n['calls']('?Check@spell_ring_reforged_rock@@')
require('?GossipHello@AI@go_ring_reforged_rock@@','?KilledMonsterCredit@Player@@')
assert {97048,40225} <= values('?GossipHello@AI@go_ring_reforged_rock@@')
require('?CreateDoor@AI@RingReforged@@','?EnableQuestScopedCollision@GameObject@@','?InheritQuestObjectScope@WorldObject@@')
assert {245122,300} <= values('?CreateDoor@AI@RingReforged@@')
assert {40225,97047,97048} <= values('?CanRewardQuest@Player@@QEAA_NPEBVQuest@@_N')
assert {40225,97047,97048,1469} <= values('?KilledMonsterCredit@Player@@')
assert {96528,2912,40225,1469} <= values('?SummonCreature@Map@@')
waypoint='?StartMove@?$WaypointMovementGenerator@VPlayer@@@@'
require(waypoint,'?Initialize@MotionMaster@@','?OnMovementInform@ScriptMgr@@')
require('?OnMovementInform@player_ring_reforged@@','?Current@RingReforged@@')
assert 473698 in values('?OnMovementInform@player_ring_reforged@@')
print('PASS final PE all7 registered Ring factories/selected hooks, actual explicit GO identity and successful native-lock personal90 receipt, private solid publication, both required reward/recipient guards and real player waypoint completion dispatch. Native fixtures establish ordering/retry/group isolation behavior; client presentation remains separate.')
