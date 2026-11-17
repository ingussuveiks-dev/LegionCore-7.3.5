"""Check the selected native Lyandra lifecycle in the final linked PE."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
p=runpy.run_path(str(Path(__file__).with_name('verify_fire_return_recovery_release.py')))
n,ins,refs=p['n'],p['ins'],p['refs']
def require(caller,*callees):
 for callee in callees:assert n['symbol'](callee) in n['calls'](caller),(caller,callee)
def values(name):return {o.imm for i in ins(name) for o in i.operands if o.type==CS_OP_IMM}
factory='?GetAI@?$GenericCreatureScript@Unpc_fire_artifact_lyandra@@@@'
require('?AddSC_artifact_acquisition@@','??0?$GenericCreatureScript@Unpc_fire_artifact_lyandra@@@@')
require(factory,'??0SmartAI@@')
vt=p['p']['data_symbol']('??_7npc_fire_artifact_lyandra@@6B@')
assert vt in refs(factory)
selected=p['slots'](vt)
for method in ('AttackStart','UpdateAI','EnterEvadeMode','JustReachedHome','Reset','JustDied'):
 assert n['symbol'](f'?{method}@npc_fire_artifact_lyandra@@') in selected,method
require('?AttackStart@npc_fire_artifact_lyandra@@','?Wreathed@FireLyandra@@','?AttackStart@SmartAI@@')
require('?UpdateAI@npc_fire_artifact_lyandra@@','?UpdateAI@SmartAI@@','?UpdateRearm@npc_fire_artifact_lyandra@@','?BindWing@npc_fire_artifact_lyandra@@','?SelectVictim@Creature@@','?getHostilTarget@ThreatManager@@','?canCreatureAttack@Creature@@','?SetInCombatWith@Unit@@','?HasCompletedLinkedCast@SmartScript@@','?AIM_Initialize@Creature@@')
assert {99615,19,15000,30000}<=values('?UpdateAI@npc_fire_artifact_lyandra@@')
require('?Scoped@FireLyandra@@','?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@','?GetCurrentStep@Scenario@@')
assert {99615,1480,957,3}<=values('?Scoped@FireLyandra@@')
assert 194360 in values('?Wreathed@FireLyandra@@')
require('?OriginalWing@FireLyandra@@','?InSamePhase@WorldObject@@','?CanTargetQuestObject@WorldObject@@','?NativeWingData@FireLyandra@@')
assert {370121,98686,33555200}<=values('?OriginalWing@FireLyandra@@')
require('?BindWing@npc_fire_artifact_lyandra@@','?WingRequest@FireLyandra@@','?GetTargets@SmartScript@@','?OriginalWing@FireLyandra@@')
require('?EnterEvadeMode@npc_fire_artifact_lyandra@@','?EnterEvadeMode@SmartAI@@','?TeardownPhase@npc_fire_artifact_lyandra@@','?CancelPendingActions@SmartScript@@')
require('?JustReachedHome@npc_fire_artifact_lyandra@@','?JustReachedHome@SmartAI@@')
reset_slot=selected.index(n['symbol']('?Reset@npc_fire_artifact_lyandra@@'))*8
assert any(i.mnemonic in {'call','jmp'} and any(o.type==CS_OP_MEM and o.mem.disp==reset_slot for o in i.operands) for i in ins('?JustReachedHome@npc_fire_artifact_lyandra@@'))
require('?CancelPendingActions@SmartScript@@','?ResetLinkedCastRetries@SmartScript@@')
require('?UpdateRearm@npc_fire_artifact_lyandra@@','?OriginalWing@FireLyandra@@','?AIM_Initialize@Creature@@','?Respawn@Creature@@')
require('?JustDied@npc_fire_artifact_lyandra@@','?JustDied@SmartAI@@')
require('?NativeSummon@FireReturnRecovery@@','?GetScriptId@ObjectMgr@@')
require('?NativeActor@FireReturnRecovery@@','?GetScriptId@ObjectMgr@@')
# Cancellation must be consumed by the native timed-list driver, not just stored.
cancel=ins('?CancelPendingActions@SmartScript@@')
assert any(o.type==CS_OP_MEM and o.size==1 and o.mem.disp==0xb9 for i in cancel for o in i.operands)
assert any(o.type==CS_OP_MEM and o.size==1 and o.mem.disp==0xb9 for i in ins('?OnUpdate@SmartScript@@') for o in i.operands)
for method in ('UpdateAI','EnterEvadeMode','Reset','JustReachedHome','JustDied','UpdateRearm','TeardownPhase','FailPhase'):
 calls=n['calls'](f'?{method}@npc_fire_artifact_lyandra@@')
 for forbidden in ('?AddItem@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?TeleportTo@Player@@','?CompleteQuest@Player@@','?RewardQuest@Player@@','?SetHealth@Unit@@','?SetCurrentStep@Scenario@@'):
  assert not any(n['resolve'](a) in calls for name,a in n['symbols'].items() if name.startswith(forbidden)),(method,forbidden)
print('PASS selected Lyandra factory/vtable, actual scenario/aura scope, original private Gorewing/native target selection, threat/combat maintenance, cast receipt, finite reset/rearm, real death delegation and return whitelist. Native timed driver consumes cancellation. No new item/progress/health/teleport grants; native host and client checks remain separate.')
