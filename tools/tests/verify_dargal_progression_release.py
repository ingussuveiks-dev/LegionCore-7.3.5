"""Verify registered Dargal service/SmartAI and native map/recipient guards in final PE."""
from pathlib import Path
import runpy,struct,re
from capstone import CS_OP_IMM
p=runpy.run_path(str(Path(__file__).with_name('verify_frost_scene_equip_release.py')))
n,ins,refs=p['n'],p['ins'],p['refs']
maptext=(n['args'].release/'worldserver.map').read_text(errors='replace')
stamp=re.search(r'Timestamp is ([0-9A-Fa-f]+)',maptext)
assert stamp and int(stamp[1],16)==struct.unpack_from('<I',n['binary'],n['pe']+8)[0]
def require(caller,*callees):
 for callee in callees:assert n['symbol'](callee) in n['calls'](caller),(caller,callee)
def anycall(caller,prefix):
 return any(n['resolve'](a) in n['calls'](caller) for name,a in n['symbols'].items() if name.startswith(prefix))
def values(name):return {o.imm for i in ins(name) for o in i.operands if o.type==CS_OP_IMM}
def slots(address):
 end=min(v for v in p['data_symbols'].values() if v>address)
 assert 16<=end-address<=8192
 return struct.unpack_from('<'+'Q'*((end-address)//8),n['binary'],n['offset'](address))
registration='?AddSC_artifact_acquisition@@'
for actor,methods in [('npc_keeping_the_peace_dargal',('Reset','IsSummonedBy','UpdateAI','JustDied')),('npc_keeping_the_peace_mortar',('IsSummonedBy','UpdateAI'))]:
 factory=f'?GetAI@?$GenericCreatureScript@U{actor}@@@@'
 script_table=p['data_symbol'](f'??_7?$GenericCreatureScript@U{actor}@@@@6BScriptObject@@@')
 assert script_table in refs(registration),(actor,'unregistered factory table')
 assert n['symbol'](factory) in slots(script_table),(actor,'unselected GetAI')
 require(factory,'??0SmartAI@@')
 table=p['data_symbol'](f'??_7{actor}@@6B@');assert table in refs(factory)
 selected=slots(table)
 for method in methods:assert n['symbol'](f'?{method}@{actor}@@') in selected,(actor,method)
ctor='??0player_keeping_the_peace@@'
table=p['data_symbol']('??_7player_keeping_the_peace@@6B@')
assert (table in refs(registration)) or (n['symbol'](ctor) in n['calls'](registration) and table in refs(ctor))
selected=slots(table)
for method in ('OnUpdate','OnBeforeLogout','OnLogout','OnMapChanged'):assert n['symbol'](f'?{method}@player_keeping_the_peace@@') in selected,method
require('?OnUpdate@player_keeping_the_peace@@','?Update@KeepingThePeace@@')
require('?Update@KeepingThePeace@@','?Pending@KeepingThePeace@@','?Tick@KeepingThePeace@@')
require('?Tick@KeepingThePeace@@','?Pending@KeepingThePeace@@','?GetQuestCancellationReceipt@Player@@','?SetQuestStatus@Player@@')
assert anycall('?Tick@KeepingThePeace@@','?CastSpell@Unit@@')
assert {42771,228199,5000,3}<=values('?Tick@KeepingThePeace@@')
assert {42771,108815,110635,196}<=values('?Pending@KeepingThePeace@@')
assert values('?Pending@KeepingThePeace@@') & {8,9}
selector='?OwnedBoss@KeepingThePeace@@'
if any(name.startswith(selector) for name in n['symbols']):
 require('?Tick@KeepingThePeace@@',selector)
else:
 selector='?Tick@KeepingThePeace@@' # Release inlines the sole owner-list consumer.
require(selector,'?Current@KeepingThePeace@@')
assert anycall(selector,'?GetCreature@Map@@')
assert {110635,228199}<=values(selector)
require('?Current@KeepingThePeace@@','?GetQuestCancellationReceipt@Player@@')
assert 42771 in values('?Current@KeepingThePeace@@')
for method in ('IsSummonedBy','UpdateAI'):
 require(f'?{method}@npc_keeping_the_peace_dargal@@','?Pending@KeepingThePeace@@','?Current@KeepingThePeace@@')
 require(f'?{method}@npc_keeping_the_peace_dargal@@',f'?{method}@SmartAI@@')
require('?JustDied@npc_keeping_the_peace_dargal@@','?JustDied@SmartAI@@','?CleanupMortars@npc_keeping_the_peace_dargal@@')
assert 5000 in values('?JustDied@npc_keeping_the_peace_dargal@@')
require('?Reset@npc_keeping_the_peace_dargal@@','?CleanupMortars@npc_keeping_the_peace_dargal@@','?Reset@SmartAI@@')
assert {111212,221411}<=values('?CleanupMortars@npc_keeping_the_peace_dargal@@')
require('?IsSummonedBy@npc_keeping_the_peace_mortar@@','?Pending@KeepingThePeace@@','?Current@KeepingThePeace@@','?SetCombatMove@SmartAI@@','?SetAutoAttack@SmartAI@@','?IsSummonedBy@SmartAI@@')
require('?UpdateAI@npc_keeping_the_peace_mortar@@','?UpdateAI@SmartAI@@','?Pending@KeepingThePeace@@')
assert {110635,111212,221411}<=values('?IsSummonedBy@npc_keeping_the_peace_mortar@@')
mapfn='?SummonCreature@Map@@'
assert {110635,228199,2912,42771,108815}<=values(mapfn)
assert anycall(mapfn,'?BindQuestObject@Player@@')
credit='?KilledMonsterCredit@Player@@'
assert {42771,110635,108815}<=values(credit)
assert values(credit) & {8,9}
for caller in ('?Tick@KeepingThePeace@@','?UpdateAI@npc_keeping_the_peace_dargal@@','?JustDied@npc_keeping_the_peace_dargal@@','?CleanupMortars@npc_keeping_the_peace_dargal@@','?UpdateAI@npc_keeping_the_peace_mortar@@'):
 for forbidden in ('?KilledMonsterCredit@Player@@','?SetQuestObjectiveData@Player@@','?CompleteQuest@Player@@','?RewardQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?SetHealth@Unit@@'):
  assert not anycall(caller,forbidden),(caller,forbidden)
print('PASS selected Dargal player hooks/two SmartAI factories/vtables,actual42771/9/228199 gate,native owner receipt and prepublication2912 binding,finite retry/cleanup/native death delegation and sequenced group recipient guard; no new credit/item/health/travel grants. Native hosts/data/client checks remain separate.')
