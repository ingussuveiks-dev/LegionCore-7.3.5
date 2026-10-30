"""Verify selected Fire recovery factories, native proof, movement and lifetime hooks."""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
p=runpy.run_path(str(Path(__file__).with_name('verify_frost_scene_equip_release.py')))
n,ins,refs=p['n'],p['ins'],p['refs']
def values(name):return {o.imm for i in ins(name) for o in i.operands if o.type==CS_OP_IMM}
def require(caller,*callees):
 targets=n['calls'](caller)
 for callee in callees:assert n['symbol'](callee) in targets,(caller,callee)
def slots(address):
 end=min(v for v in p['data_symbols'].values() if v>address)
 assert 16<=end-address<=8192
 return struct.unpack_from('<'+'Q'*((end-address)//8),n['binary'],n['offset'](address))
registration='?AddSC_artifact_acquisition@@'
player_ctor='??0player_fire_artifact_return@@'
factory_ctor='??0?$GenericCreatureScript@Unpc_fire_artifact_aethas@@@@'
factory='?GetAI@?$GenericCreatureScript@Unpc_fire_artifact_aethas@@@@'
require(registration,player_ctor,factory_ctor)
player_table=p['data_symbol']('??_7player_fire_artifact_return@@6B@')
assert player_table in refs(player_ctor)
selected=slots(player_table)
for method in ('OnUpdate','OnLogout','OnMapChanged'):assert n['symbol'](f'?{method}@player_fire_artifact_return@@') in selected
require('?OnUpdate@player_fire_artifact_return@@','?Update@FireReturnRecovery@@')
for method in ('OnLogout','OnMapChanged'):require(f'?{method}@player_fire_artifact_return@@','?Forget@FireReturnRecovery@@')
ai_table=p['data_symbol']('??_7npc_fire_artifact_aethas@@6B@')
assert ai_table in refs(factory)
selected=slots(ai_table)
for method in ('SetData','Reset'):assert n['symbol'](f'?{method}@npc_fire_artifact_aethas@@') in selected
assert n['symbol']('?UpdateAI@SmartAI@@') in selected
assert selected[0x130//8]==n['symbol']('?JustDied@SmartAI@@')
require(factory,'??0SmartAI@@')
require('?SetData@npc_fire_artifact_aethas@@','?SetData@SmartAI@@','?GetScript@SmartAI@@','?HasEscortState@SmartAI@@')
require('?Reset@npc_fire_artifact_aethas@@','?Reset@SmartAI@@')
assert 520 in values(factory),'Selected AI allocation does not include its return lifecycle state'
reset=ins('?Reset@npc_fire_artifact_aethas@@')
assert any(i.mnemonic in {'inc','add'} and any(o.type==CS_OP_MEM and o.size==4 and o.mem.disp==0x204 for o in i.operands) for i in reset),'Reset epoch is not advanced'
assert any(i.mnemonic=='mov' and any(o.type==CS_OP_MEM and o.size==1 and o.mem.disp==0x200 for o in i.operands) and any(o.type==CS_OP_IMM and o.imm==0 for o in i.operands) for i in reset),'Pending native speech is not cleared on Reset'
assert {3,4,7,38,40}<=values('?SetData@npc_fire_artifact_aethas@@')
earned='?Earned@FireReturnRecovery@@';tick='?Tick@FireReturnRecovery@@';recover='?Recover@npc_fire_artifact_aethas@@'
require(earned,'?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@','?GetCurrentStep@Scenario@@','?GetAchievementMgr@Scenario@@','?GetCriteriaProgressMap@?$AchievementMgr@VScenario@@@@')
assert {1480,957,4,5,45965}<=values(earned),'Actual earned instance/death leaf differs'
require('?Update@FireReturnRecovery@@',earned,tick)
require(tick,earned,'?NativeSummon@FireReturnRecovery@@','?NativeAethas@FireReturnRecovery@@','?Find@FireReturnRecovery@@','?NativeActor@FireReturnRecovery@@',recover,'?SummonCreature@WorldObject@@QEAAPEAVTempSummon@@IMMMMW4TempSummonType@@')
# The shared C++ symbol reader intentionally reads only decorated names.
# This CRT import has an undecorated public/import pair; use its exact map
# records and the actual call operand instead of a C++ prefix lookup.
crt_map=(n['args'].release/'worldserver.map').read_text(errors='replace')
crt_import=re.findall(r'^\s+\w+:\w+\s+__imp___RTDynamicCast\s+([0-9A-Fa-f]{16})\s',crt_map,re.M)
crt_public=re.findall(r'^\s+\w+:\w+\s+__RTDynamicCast\s+([0-9A-Fa-f]{16})\s+f',crt_map,re.M)
assert len(crt_import)==len(crt_public)==1
assert (int(crt_import[0],16) in refs(tick) or n['resolve'](int(crt_public[0],16)) in n['calls'](tick)),'Actual AI type guard is missing'
assert {500,30000,99615,99763,99861}<=values(tick)
require('?Find@FireReturnRecovery@@','?GetCreatureListWithEntryInGrid@WorldObject@@','?InSamePhase@WorldObject@@','?CanTargetQuestObject@WorldObject@@','?IsWithinDist3d@WorldObject@@QEBA_NMMMM_N@Z')
require(recover,earned,'?NativeAethas@FireReturnRecovery@@','?Find@FireReturnRecovery@@','?GetScript@SmartAI@@','?StartPath@SmartAI@@','?HasEscortState@SmartAI@@','?SummonCreature@WorldObject@@QEAAPEAVTempSummon@@IMMMMW4TempSummonType@@','?ProcessAction@SmartScript@@')
assert {4,6,7,9,38,40,99763,99861}<=values(recover)
assert any(o.type==CS_OP_MEM and o.size==1 and o.mem.disp==0x99 for i in ins(recover) for o in i.operands),'Real runOnce/arrival proof no longer read'
require('?NativeAethas@FireReturnRecovery@@','?GetScriptId@ObjectMgr@@','?GetCreatureTemplate@ObjectMgr@@','?GetScript@SmartAIMgr@@')
require('?NativeSummon@FireReturnRecovery@@','?GetScript@SmartAIMgr@@','?GetCreatureTemplate@ObjectMgr@@')
for caller in (earned,tick,recover,'?Update@FireReturnRecovery@@','?SetData@npc_fire_artifact_aethas@@'):
 targets=n['calls'](caller)
 for forbidden in ('?CastSpell@Unit@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?TeleportTo@Player@@','?CompleteQuest@Player@@','?RewardQuest@Player@@','?SetHealth@Unit@@','?SetCurrentStep@Scenario@@'):
  assert not any(n['resolve'](address) in targets for name,address in n['symbols'].items() if name.startswith(forbidden)),(caller,forbidden)
for name in (b'player_fire_artifact_return',b'npc_fire_artifact_aethas'):assert name in n['binary']
print('PASS actual selected Fire PlayerScript/vtables and derived SmartAI factory/Reset/SetData/native death/update; real957/4..5 and positive45965 map, native actor lookup/summon/path and actual runOnce/waypoint proof,500ms/30000ms finite state. No direct progress/item/health/teleport grants or added cast. Native host owns failed/reentrant/context cases; actual client movement/return remains separate.')
