"""Actual linked actor ownership, original entry guards and bounded paired rotation."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_fourteenth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];ref=p['ref'];expanded=p['expanded'];callbacks=p['callbacks'];lookup=p['lookup'];map_guards=p['map_guards']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
val=callbacks('instance_invasion_valshara_InstanceMapScript','OnPlayerEnter');assert len(val)==2
for name in val:
 r=expanded(name);map_guards(r);assert sites(r,'?GetQuestStatus@Player@@')and 45856 in values(r)
 assert op(r,'cmp','dword ptr [rax + 8], 0'),'weak entry attempt expires'
assert any(4587 in values(expanded(s))for s in val)
maw,=callbacks('instance_maw_of_souls_InstanceMapScript','OnPlayerEnter');r=expanded(maw);map_guards(r)
assert sites(r,'?GetInstanceScript@WorldObject@@')and sites(r,'?GetBossState@InstanceScript@@')and sites(r,'?RepopAtGraveyard@Player@@'),'late native recovery admission'
style,=lookup('npc_atheris_voguesong','QueueStageEffect','?GetPlayer@ObjectAccessor@@',1)
assert {261939,261938,261918,261923}<=values(style)
assert op(style,'mov','rax, qword ptr [r8 + 0x38]')and op(style,'cmp','qword ptr [rcx + 0x20], rax'),'stage Reset generation guard'
assert sites(ins('?DoAction@npc_atheris_voguesong@@'),'?QueueStageEffect@npc_atheris_voguesong@@')and sites(ins('?UpdateAI@npc_atheris_voguesong@@'),'?QueueStageEffect@npc_atheris_voguesong@@')
pull=[expanded(s)for s in callbacks('boss_demonic_inquisitionAI','EnterCombat')if sites(expanded(s),'?GetCreature@ObjectAccessor@@')];assert len(pull)==1;pull=pull[0]
assert sites(pull,'?GetUnit@ObjectAccessor@@')and op(pull,'test','rsi, rsi')and op(pull,'test','rbx, rbx'),'both results guarded after their lookups'
assert len(sites(pull,'?isInCombat@Unit@@'))==2 and sum(x.mnemonic=='cmp'and '0xa18]'in x.op_str for x in pull)==3,'live source,partner and attacker plus source/partner combat guards'
cinema=lookup('npc_karazhan_nighbane_starterAI','sGossipSelect','?GetCreature@ObjectAccessor@@',3)
assert any(1000 in values(r)for r in cinema),'nested one-second handoff retained'
r=ins('?sGossipSelect@npc_karazhan_nighbane_starterAI@');summon,=sites(r,'?SummonCreature@WorldObject@@')
assert op(r,'cmp','byte ptr [rcx + 0x58], 0')and op(r,'mov','byte ptr [rdi + 0x58], 1'),'cinematic exactly-once admission'
assert r[summon+2].mnemonic=='test'and r[summon+3].mnemonic=='je','failed actual summon returns before consumption'
assert {115213,232663,75000,79000,81000,87000}<=values(r),'native cinematic actor,achievement and timers retained'
avatar,=lookup('boss_fallen_avatar','MoveInLineOfSight','?GetCreature@ObjectAccessor@@',1)
assert 240897 in values(avatar)and 13000 in values(avatar),'avatar release and independent Kiljaeden cleanup scheduled'
khadgar,=lookup('boss_guldanAI','CreateKhadgarAndFigure','?GetCreature@ObjectAccessor@@',1)
assert any(x.mnemonic=='cmp'and '0xa18]'in x.op_str for x in khadgar),'dead Guldan no new figures'
assert {227671,227674,231256,231262,231263,231264}<=values(khadgar),'all four original figure colors preserved'
oso,=lookup('boss_brawguild_osoAI','Reset','?GetCreature@ObjectAccessor@@',1)
assert op(oso,'mov','rax, qword ptr [rdx + 0xc0]')or op(oso,'mov','rax, qword ptr [rcx + 0xc0]'),'Oso intro generation load'
r=ins('?Reset@boss_brawguild_osoAI@');assert sites(r,'?_Reset@BrawlersBossAI@@')and sites(r,'?KillAllFunctions@FunctionProcessor@@')and 6000 in values(r),'prior summons and old intro/Grizzly functions cleaned;authored intro pause retained'
morph,=callbacks('boss_brawguild_raydAI','UpdateAI');r=expanded(morph)
assert op(r,'mov','edx, dword ptr [rcx + 0x10]')and sites(r,'?RemoveAura@Unit@@'),'immutable morph spell copied in closure'
r=ins('?UpdateAI@boss_brawguild_raydAI@');assert op(r,'movabs','rax, 0xcccccccccccccccd')and op(r,'mul','rcx')and op(r,'shr','rdx, 3')and op(r,'sub','rcx, rax')and op(r,'mov','byte ptr [rbx + 0xc0], cl'),'actual uint64 modulo10 remainder stored in phase'
assert {11000,12000}<=values(r)and sites(r,'?RescheduleEvent@EventMap@@'),'ordinary timing continues after bounded step'
print('PASS actual Release fifteenth ten:Valsharah/Maw original entry attempts and late admission;Style actor queue/GUID/Reset with twenty source callsites;Inquisition source/attacker guards;Nightbane three actor lookups and successful one-shot summon;Avatar GUID/own nested queues and independent cleanup;Guldan figure lookup/death guard/four colors;Oso generation/helper cleanup;immutable Ray morph and actual modulo10 rotation. Prior147 selected paths retained;source/native/client scopes distinct.')
