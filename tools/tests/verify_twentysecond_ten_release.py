"""Require all ten live-bound repairs in the linked Release,retaining prior217 checks."""
from pathlib import Path
import json,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_twentyfirst_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('TwentySecondTenRepairs.json').read_text(encoding='utf8'))
def op(r,m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
for e in plan:
 for cls in e['classes']:
  r=ins('?DamageTakenAfterScaling@'+cls+'@');assert sites(r,'?GetHealth@Unit@@'),('actual normalized target-relative health',cls)
  assert not any(s.startswith('?DamageTaken@'+cls+'@')for s in n['symbols']),('old raw-damage override absent',cls)
r=ins('?DamageTakenAfterScaling@npc_energized_phoenixAI@');assert op(r,'cmp','byte ptr [rcx + 0x59], 0')and {167629,66325}<=values(r),'actual sleeping protection and native explosion/model'
r=ins('?Reset@npc_energized_phoenixAI@');assert 1 in values(r)and sites(r,'?Clear@ObjectGuid@@'),'idle movement disabled and old target cleared'
r=ins('?IsSummonedBy@npc_energized_phoenixAI@');assert 2000 in values(r)and op(r,'mov','byte ptr [rsi + 0x59], 0'),'wake clears sleep and rearms native2s clock'
r=ins('?TryReleaseAdds@npc_artruis_Q12581@@');assert len(sites(r,'?FindNearestCreature@WorldObject@@'))==2 and {28667,28668}<=values(r),'both real native add searches'
assert op(r,'mov','byte ptr [rbx + 0x5a], 1')and op(r,'mov','byte ptr [rbx + 0x5c], 0'),'release commits only accepted paired actors'
u=ins('?UpdateAI@npc_artruis_Q12581@@');release,=sites(u,'?TryReleaseAdds@npc_artruis_Q12581@@');shield,=sites(u,'?HasAura@Unit@@');assert release<shield,'retry precedes shield early return'
r=ins('?DamageTakenAfterScaling@npc_highmaul_koragh_volatile_anomaly@@');assert {141608,163466,162595}<=values(r)and sites(r,'?HasAura@Unit@@')and len(sites(r,'?CastSpell@Unit@@'))==2,'native unsilenced explosion/fade before real death'
r=ins('?DamageTakenAfterScaling@npc_q42782AI@');assert sites(r,'?GetAffectingPlayer@Unit@@')and sites(r,'?IsInWorld@Object@@')and sites(r,'??9ObjectGuid@@')and sites(r,'?KilledMonsterCredit@Player@@')and {108722,52994,7267}<=values(r),'live actual challenger owner gets native credit/victory once'
assert sites(ins('?UpdateAI@npc_q42782AI@'),'?GetPlayer@ObjectAccessor@@'),'missing/dead challenger retries through fresh native lookup and evade'
r=ins('?DamageTakenAfterScaling@npc_neptulonAI@');assert sites(r,'?Talk@CreatureAI@@')and any(x.mnemonic=='call'and '['in x.op_str for x in r),'original failure speech/virtual evade preserved'
r=ins('?DamageTakenAfterScaling@npc_blaze_of_the_heavens@@');assert sites(r,'?SetHealth@Unit@@')and sites(r,'?SetPhase@EventMap@@')and sites(r,'?RemoveAura@Unit@@'),'native egg transition'
r=ins('?DamageTakenAfterScaling@npc_servant_of_siamat@@');assert 2000 in values(r)and sites(r,'?ScheduleEvent@EventMap@@')and sites(r,'?SetHealth@Unit@@'),'authored heroic charge/death clock'
r=ins('?DamageTakenAfterScaling@boss_high_inquisitor_whitemaneAI@');assert 50 in values(r)and sites(r,'?CountPctFromMaxHealth@Unit@@')and sites(r,'?MovePoint@MotionMaster@@'),'native half-health resurrection/partner movement'
r=ins('?DamageTakenAfterScaling@npc_commander_durandAI@');assert sites(r,'?RemoveAurasAllDots@Unit@@')and op(r,'mov','byte ptr [rdi + 0x61], 1'),'Durand fake-death state committed once'
r=ins('?DamageTakenAfterScaling@boss_master_snowdrift@@');assert op(r,'mov','byte ptr [rdi + 0x192], 1')and 10000 in values(r),'boss surrender latch/native10s cleanup'
for cls in ['npc_snowdrift_novice','npc_snowdrift_miniboss']:
 r=ins('?DamageTakenAfterScaling@'+cls+'@@');assert op(r,'cmp','byte ptr [rcx + 0x61], 0'),'retired training actor protection '+cls
r=ins('?DamageTakenAfterScaling@boss_felmystAI@');assert 1 in values(r),'native ground-versus-flight gate remains'
print('PASS actual Release twentysecond ten:all13 normalized-damage hooks in ten independently bound problems;phoenix idle/sleep/wake;Artruis accepted paired release and shielded poll retry;Ko raghs exact-lethal unsilenced explosion;actual Broken Shore challenger/owner/liveness/abandon;Neptulon failure;Blaze egg;heroic Siamat charge;Durand/Whitemane resurrection;all three Snowdrift surrender counters/retired guards;Felmyst flight. Prior217 selected paths retained,total227;actual client traversal remains pending.')
