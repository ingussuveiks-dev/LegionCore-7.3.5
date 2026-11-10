"""Check actual linked callback captures, origin guards and credit admission."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM
p=runpy.run_path(str(Path(__file__).with_name('verify_thirteenth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values'];expanded=p['expanded'];callbacks=p['callback_names'];lookup=p['lookup_callbacks'];map_guards=p['map_guards']
molten=lookup('npc_felhounds_fharg','UpdateAI','?GetCreature@ObjectAccessor@@',1)[0];assert 249227 in values(molten)
assert any(x.mnemonic=='cmp'and x.op_str.endswith('+ 0xa18], 0')for x in molten),'dead-boss combat guard'
route,=callbacks('boss_brawguild_masterAI','JustSummoned');r=expanded(route)
assert sites(r,'?MovePath@MotionMaster@@')
assert not any(x.mnemonic=='call'and n['symbols'].get('?GetTeamId@Player@@')==n['resolve'](int(x.op_str,16))for x in r if x.op_str.startswith('0x'))
assert any(x.mnemonic=='cmp'and x.op_str=='byte ptr [rbx + 0x10], cl'for x in r),'faction captured as value'
assert 5 in values(r)and len(sites(r,'?urand@@'))==2,'original bounded path selection'
spider=lookup('spell_admiral_ripsnarl_coalesce_SpellScript','HandleAfterCast','?GetCreature@ObjectAccessor@@',1)[0]
assert sites(spider,'?GetUnit@ObjectAccessor@@')and sites(spider,'?MoveJump@MotionMaster@@')and 116894 in values(spider)and 3000 in values(spider),'cocoon release/caster lookup/cleanup/jump'
for cls,q,conversation in [('instance_invasion_stormheim_InstanceMapScript',46110,4527),('instance_InvasionAzsuna_InstanceMapScript',46199,4587),('instance_invasion_HM_InstanceMapScript',46182,4676)]:
 names=callbacks(cls,'OnPlayerEnter');assert len(names)==2
 for name in names:
  r=expanded(name);map_guards(r)
  assert any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rax + 8], 0'for x in r),'expired entry attempt rejected'
  assert q in values(r)and sites(r,'?GetQuestStatus@Player@@'),'current native quest recheck'
 assert any(conversation in values(expanded(name))for name in names)
 leave=ins('?OnPlayerLeave@'+cls+'@');assert leave,'actual instance leave override linked'
assault=callbacks('instance_AssaultBrokenShore_InstanceMapScript','OnPlayerEnter');assert len(assault)==2
for name in assault:map_guards(expanded(name));assert any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rax + 8], 0'for x in expanded(name))
assert any(4526 in values(expanded(name))for name in assault)
night=callbacks('instance_the_nightnold_InstanceMapScript','OnPlayerEnter');assert len(night)==2
for name in night:map_guards(expanded(name))
floor=[expanded(s)for s in night if sites(expanded(s),'?NearTeleportTo@Unit@@')];assert len(floor)==1 and sites(floor[0],'?IsWithinBox@Position@@')and sites(floor[0],'?GetBossState@InstanceScript@@')
intro=[expanded(s)for s in night if sites(expanded(s),'?CreateConversation@Conversation@@')];assert len(intro)==1
r=intro[0];create,=sites(r,'?CreateConversation@Conversation@@');assert r[create+1].mnemonic=='test'and r[create+2].mnemonic=='jne';assert any(x.mnemonic=='mov'and x.op_str=='byte ptr [rdi + 0x388], 1'for x in r[create:]),'commit intro only after successful native creation'
telarn=[s for s in callbacks('boss_high_botanist_telarn','')if '???0boss_high_botanist_telarn' in s];assert len(telarn)==1
r=expanded(telarn[0]);assert any(x.mnemonic=='cmp'and x.op_str.endswith('+ 0xa18], 0')for x in r),'constructor callback alive admission'
factory=ins('?GetAI@?$GenericCreatureScript@Uboss_high_botanist_telarn@@@@')
# The lifetime anchor can move this formerly inlined constructor out of GetAI.
# Follow its actual factory call;keep the original500ms/Unit queue proof.
ctor=ins('??0boss_high_botanist_telarn@@')if sites(factory,'??0boss_high_botanist_telarn@@')else factory
assert sites(ctor,'?AddDelayedEvent@FunctionProcessor@@')and 500 in values(ctor),'actual creature function queue initialization'
inline_queue=any(x.mnemonic=='mov'and x.op_str=='rcx, qword ptr [rdi + 0x20]'for x in ctor)and any(x.mnemonic=='add'and x.op_str=='rcx, 0x36a0'for x in ctor)
outlined_queue=any(x.mnemonic=='mov'and x.op_str=='rdx, qword ptr [rbx + 0x20]'for x in ctor)and any(x.mnemonic=='lea'and x.op_str=='rcx, [rdx + 0x36a0]'for x in ctor)
assert inline_queue or outlined_queue,'constructor queues on actual Creature Unit::m_Functions'
r=ins('?UpdateAI@npc_dramock@@');assert sites(r,'?GetPlayer@ObjectAccessor@@')and sites(r,'?GetQuestStatus@Player@@')and sites(r,'?GetReqKillOrCastCurrentCount@Player@@')and sites(r,'?KilledMonsterCredit@Player@@')and 47430 in values(r),'native current quest/objective/credit dispatch'
snapshot,=callbacks('npc_dramock','MoveInLineOfSight');r=expanded(snapshot);assert any(x.mnemonic=='mov'and x.op_str=='rax, qword ptr [rcx + 0x70]'for x in r)and any(x.mnemonic=='cmp'and 'rax'in x.op_str for x in r),'pending snapshot generation checked'
assert any(x.mnemonic=='inc'and '0x70]'in x.op_str for x in ins('?Reset@npc_dramock@@')),'Reset invalidates pending snapshot'
print('PASS actual Release fourteenth ten:Molten GUID/dead-boss guard;immutable faction route;Cathedral two actor lookups/release/cleanup;journal-free weak entry attempts and origin guards in three invasion,two Assault and two Nighthold stages;native quest rechecks,Nighthold late floor/create receipt;Telarn actor queue and alive gate;Dramock generation/current player quest/objective/native credit. Prior137 selected paths retained;native/source/client scopes distinct.')
