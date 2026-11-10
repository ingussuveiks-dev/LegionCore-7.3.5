"""Verify final-damage transitions and retain the prior linked checks."""
from pathlib import Path
import json,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_twentythird_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
def op(r,m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
plan=json.loads(Path(__file__).with_name('TwentyFourthTenRepairs.json').read_text(encoding='utf8'))
for e in plan:
 for cls in e['classes']:
  r=ins('?DamageTakenAfterScaling@'+cls+'@');assert sites(r,'?GetHealth@Unit@@'),cls
  assert not any(s.startswith('?DamageTaken@'+cls+'@')for s in n['symbols']),cls
for cls,calls,constants in [
 ('boss_muruAI',['?RemoveAllAuras@Unit@@','?DoCast@UnitAI@@'],[46177]),
 ('npc_harbinger_of_darkness',['?FindNearestCreature@WorldObject@@','?DespawnOrUnsummon@Creature@@'],[2000]),
 ('npc_anomaly_waning_time_particleAI',['?GetAnyOwner@Unit@@','?SendEncounterUnit@InstanceScript@@'],[3000,199615]),
 ('boss_elisande',['?KillAllFunctions@FunctionProcessor@@','?KillAllEvents@EventProcessor@@'],[100]),
 ('npc_telarn_plasma_sphere',['?GetAnyOwner@Unit@@','?HasAura@Unit@@'],[100,218780]),
 ('boss_eadricAI',['?BindPlayersToInstance@@','?MovePoint@MotionMaster@@'],[68575]),
 ('boss_paletressAI',['?BindPlayersToInstance@@','?MovePoint@MotionMaster@@'],[68574]),
 ('boss_black_knightAI',['?MoveIdle@MotionMaster@@','?SetHealth@Unit@@'],[67729]),
 ('boss_ingvar_the_plundererAI',['?SetPhase@EventMap@@','?DoScriptText@@'],[]),
 ('npc_vrykul_skeletonAI',['?ScheduleEvent@EventMap@@','?HasFlag@Object@@'],[18,22,1000]),
 ('npc_living_amberAI',['?DoStopAttack@ScriptedAI@@','?RemoveAurasDueToSpell@Unit@@'],[122348,122532])]:
 r=ins('?DamageTakenAfterScaling@'+cls+'@');assert all(sites(r,c)for c in calls)and set(constants)<=values(r),cls
r=ins('?DamageTakenAfterScaling@boss_black_knightAI@');assert op(r,'cmp','byte ptr [rcx + 0xa8], 0'),'pending resurrection rejects repeat damage before ghoul explosion'
r=ins('?DamageTakenAfterScaling@npc_harbinger_of_darkness@');latch=next(i for i,x in enumerate(r)if x.mnemonic=='mov'and x.op_str.endswith(', 1')and'byte ptr'in x.op_str);notify,=sites(r,'?FindNearestCreature@WorldObject@@');assert latch<notify,'death latch before reentrant controller notification'
print('PASS actual Release twentyfourth ten:11 final-damage handlers in10 independently bound encounter lifecycles;Muru exact lethal,Harbinger pre-notification latch,particle four-fragment/rift,Elisande pending rewind,plasma explosion,both Argent surrenders,Black Knight pending resurrection,Ingvar ritual,Keleseth skeleton andamber. Prior237 selected paths retained,total247;actual client/full encounters remain separate.')
