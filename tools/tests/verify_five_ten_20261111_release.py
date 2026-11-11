from five_ten_20261111_scope import PLAN,require_fixed
from five_ten_20261111_pe import n,ins,sites,values,callbacks,expanded
from verify_ai_lifetime_guard import verify_plan
import re
require_fixed()
p=dict(n=n,ins=ins,sites=sites,values=values,callbacks=callbacks,expanded=expanded)
seen=verify_plan(p,[dict(key='Meatball',callbacks=[dict(cls='boss_brawguild_meatballAI',method='JustDied'),dict(cls='boss_brawguild_meatballAI',method='JustDied')])]);assert len(seen)==2
for e in PLAN['changes']:
 if e['kind']!='death':continue
 rows=ins('?JustDied@'+e['cls']+'@');index=e['death_index']
 if index in (0,1,2,6,14,15,23):assert sites(rows,'?GetAffectingPlayer@Unit@@'),e['key']
 if index in (3,4,12,13,16,17):assert sites(rows,'?Kill@Unit@@'),e['key']
 if index in (24,25,26):
  at,=sites(rows,'?GetInstanceScript@WorldObject@@');aliases={'rcx'}
  for x in rows[:at]:
   match=re.fullmatch(r'(r\w+), (r\w+)',x.op_str)if x.mnemonic=='mov'else None
   if match and match[2]in aliases:aliases.add(match[1])
  assert any(x.mnemonic=='mov'and any(x.op_str=='rcx, qword ptr ['+reg+' + 0x20]'for reg in aliases)for x in rows[max(0,at-5):at]),(e['key'],'victim me supplies instance')
  assert sites(rows,'?DoUpdateAchievementCriteria@InstanceScript@@'),e['key']
 if index==19:
  at,=sites(rows,'?MoveFall@MotionMaster@@');assert any(x.mnemonic=='xor'and x.op_str=='edx, edx'for x in rows[max(0,at-5):at]),'Actual pyrite movement ID must be native zero'
print('PASS linked24complete death hooks:actual owner/native Kill/victim-instance/pyrite MoveFall endpoints and two Meatball closure admission gates')
checks=[('npc_q39685AI','OnSpellClick'),('npc_drakuru_shacklesAI','SpellHit'),('npc_argus_gift_of_the_lifebinder','SpellHitTarget'),('npc_gizmoAI','MoveInLineOfSight'),('npc_invasion_azsuna_dragonsAI','OnSpellClick')]
for cls,method in checks:
 rows=ins('?'+method+'@'+cls+'@');assert any(x.mnemonic=='cmp'and re.search(r'\+ 0x16\], 4$',x.op_str)for x in rows),(cls,'actual player type admission')
gizmo=ins('?MoveInLineOfSight@npc_gizmoAI@');assert sites(gizmo,'?SummonCreature@Map@@'),'Actual Doc summon remains'
assert 22 in values(ins('?UpdateAI@go_cheer_speaker@@')),'Fireworks inclusive maximum22'
assert 6 in values(ins('?HandleOnCast@spell_throw_staff_SpellScript@')),'ThrowStaff inclusive maximum6'
for cls,method in [('boss_brawguild_shadowmasterAI','UpdateAI'),('boss_brawguild_johnnyAI','UpdateAI'),('npc_brawguild_thwack_u_controllerAI','UpdateAI')]:
 rows=ins('?'+method+'@'+cls+'@');assert sites(rows,'?RescheduleEvent@EventMap@@')or sites(rows,'?ScheduleEvent@EventMap@@'),cls
 lookups=sites(rows,'?FindCreature@ObjectAccessor@@')or sites(rows,'?GetCreature@Map@@');assert lookups,cls
 assert any(any(x.mnemonic=='cmp'and 'qword ptr'in x.op_str for x in rows[max(0,at-16):at])and any(x.mnemonic in ('je','jz')and x.operands[0].imm>rows[at].address for x in rows[max(0,at-16):at])for at in lookups),(cls,'empty-list branch skips lookup while retaining timers')
for cls in ('spell_rog_cheat_death_AuraScript','spell_dh_disable_absorb_AuraScript','spell_hodir_protective_gaze_AuraScript'):
 rows=ins('?Absorb@'+cls+'@');at,=sites(rows,'?CountPctFromMaxHealth@Unit@@');window=rows[at+1:at+12]
 assert any(x.mnemonic=='cmp'and re.fullmatch(r'r\w+, r\w+',x.op_str)for x in window),(cls,'wide threshold compare')
 assert not any(x.mnemonic=='mov'and re.fullmatch(r'e\w+, eax',x.op_str)for x in window),(cls,'threshold cannot narrow to32')
for cls in ('npc_xakal_fissureAI','npc_bethtilac_engorged_broodlingAI'):assert any('??_7'+cls+'@'in name for name in n['symbols'])or any(cls+'@'in name for name in n['symbols']),cls
reset=ins('?Reset@npc_bethtilac_engorged_broodlingAI@');assert any(x.mnemonic=='mov'and x.op_str.startswith('byte ptr [rcx +')and x.op_str.endswith(', 0')for x in reset),'Broodling reset rearms actual latch'
assert not any(name.startswith('?EnterCombat@npc_bethtilac_engorged_broodlingAI@')for name in n['symbols']),'Entering combat must not reopen an already consumed burst'
matches=[expanded(name)for name in callbacks('instance_antorus_InstanceMapScript','OnGameObjectCreate')];assert any(sites(rows,'?GetGameObject@Map@@')for rows in matches),'Elevator delayed current GUID lookup'
matches=[expanded(name)for name in callbacks('instance_antorus_InstanceMapScript','SetData')];assert any(sites(rows,'?HandleGameObject@InstanceScript@@')and any(x.mnemonic=='movups'and x.op_str=='xmm0, xmmword ptr [rcx + 0x10]'for x in rows)for rows in matches),'Torment copies its embedded full128GUID into the native handler'
for method in ('HealthAbovePct','HealthBelowPct'):
 rows=ins('?'+method+'@boss_valithria_dreamwalkerAI@');assert len(sites(rows,'??$eval_multiply@'))>=2 and {0x38,0x40,100}<=values(rows),'Valithria compares actual64fields with128products'
rows=ins('?UpdateAI@boss_vanessa_vancleefAI@');assert len(sites(rows,'??$eval_multiply@'))>=3,'Vanessa keeps all three128product/division phase checks'
rows=ins('?DoAction@boss_ichoronAI@');assert len(sites(rows,'?ModifyHealth@Unit@@'))==2 and any(x.mnemonic=='neg'and x.op_str=='rdx'for x in rows),'Ichoron native signed64loss and positive heal'
rows=ins('?Reset@boss_murmurAI@');at,=sites(rows,'?SetHealth@Unit@@');assert any(x.mnemonic=='mov'and x.op_str=='rdx, rax'for x in rows[max(0,at-6):at]),'Murmur forwards full native percentage result'
rows=ins('?UpdateAI@mob_figment_of_doubt_AI@');assert sites(rows,'?HealBySpell@Unit@@'),'Actual Sha full-width heal backend'
rows=ins('?UpdateAI@boss_lord_rhyolithAI@');assert sites(rows,'?SetHealth@Unit@@')and sites(rows,'?SummonCreature@WorldObject@@'),'Actual Rhyolith phase summon/native health endpoint retained'
for method,cls in [('OnPeriodic','spell_xt002_gravity_bomb_aura_AuraScript'),('HandleScript','spell_xt002_gravity_bomb_damage_SpellScript')]:
 rows=ins('?'+method+'@'+cls+'@');assert any(x.mnemonic=='cmp'and re.fullmatch(r'r\w+, r\w+',x.op_str)for x in rows),'Actual Gravity Bomb full-width target health comparison'
print('PASS selected linked5x10 admission,bounds,container timers,wide survival cutoffs,latch reset and Antorus GUID callback consumers;source/native/ASAN scopes remain explicit. Whole encounter/client traversal remains pending.')
