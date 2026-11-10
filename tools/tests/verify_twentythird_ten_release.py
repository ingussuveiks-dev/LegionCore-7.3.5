"""Verify ten linked lethal lifecycles and actor-owned immutable deferred work."""
from pathlib import Path
import json,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_twentysecond_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
def op(r,m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
plan=json.loads(Path(__file__).with_name('TwentyThirdTenRepairs.json').read_text(encoding='utf8'))
for e in plan:
 r=ins('?DamageTakenAfterScaling@'+e['cls']+'@');assert sites(r,'?GetHealth@Unit@@'),e['cls'];assert not any(s.startswith('?DamageTaken@'+e['cls']+'@')for s in n['symbols']),e['cls']
r=ins('?DamageTakenAfterScaling@npc_aggramar_ember_of_taeshalach@@');assert sites(r,'?IsHeroicPlusRaid@ScriptedAI@@')and op(r,'mov','byte ptr [rdi + 0x39], 1')and 245905 in values(r),'heroic remnants preserved'
r=ins('?ReoriginationPulse@npc_argus_reorigination_module@@');radius,=sites(r,'?GetRadius@AreaTrigger@@');cast,=sites(r,'?CastSpell@Unit@@');assert radius<cast and op(r,'mov','byte ptr [rcx + 0x39], 1'),'radius snapshot before possibly synchronous cast completion and one pulse'
cb,=callbacks('npc_argus_reorigination_module','SpellFinishCast');c=expanded(cb);assert sites(c,'?GetAreaObject@Unit@@')and sites(c,'?SetSphereScale@AreaTrigger@@')and op(c,'mov','rcx, qword ptr [rcx + 8]')and op(c,'movss','xmm1, dword ptr [rbx + 0x10]')and 1000 in values(c),'module pointer and immutable radius captured without AI lookup'
assert 100 in values(ins('?SpellFinishCast@npc_argus_reorigination_module@@')),'authored100ms combat queue retained'
r=ins('?DamageTakenAfterScaling@boss_eonar@@');assert op(r,'mov','byte ptr [rdi + 0xc8], 1')and sites(r,'??$DoAction@VEntryCheckPredicate@@@SummonList@@')and 124445 in values(r),'one Paraxis wipe action'
r=ins('?DamageTakenAfterScaling@npc_worldbreaker_annihilation_decimator@@');assert sites(r,'?GetAnyOwner@Unit@@')and sites(r,'?IsMythicRaid@ScriptedAI@@')and {122773,122778,246897,246965,11686}<=values(r)and op(r,'mov','byte ptr [rdi + 0x40], 1'),'both weapon paths/owner and mythic haywire retained'
for method in ['Reset','DamageTakenAfterScaling']:
 r=ins('?'+method+'@npc_hyrja_defendersAI@');assert op(r,'cmp','byte ptr [rax + 0x16], 3')and op(r,'cmp','byte ptr [rax + 0x858], 0'),'actual creature/enabled-AI owner guard '+method
assert op(ins('?SpiritForm@npc_hyrja_defendersAI@'),'mov','byte ptr [rcx + 0x58], 1'),'restored spirit commits ascendance too'
r=ins('?DamageTakenAfterScaling@npc_ilgynoth_nightmare_ichor@@');assert 3000 in values(r)and 209471 in values(r)and op(r,'mov','byte ptr [rdi + 0x58], 1'),'one native Ichor explosion/3s cleanup'
cb,=callbacks('npc_tos_soul_residue','DamageTakenAfterScaling');c=expanded(cb);assert sites(c,'?DespawnOrUnsummon@Creature@@')and op(c,'mov','rcx, qword ptr [rcx + 8]')and not any(x.mnemonic=='mov'and x.op_str=='rcx, qword ptr [rcx + 0x20]'for x in c),'residue callback resolves captured actor directly instead of retired AI'
r=ins('?DamageTakenAfterScaling@npc_tos_soul_residue@@');assert 5000 in values(r)and {236158,236160}<=values(r)and op(r,'mov','byte ptr [rdi + 0x40], 1'),'native eruption/notification once and5s cleanup'
r=ins('?DamageTakenAfterScaling@npc_festerface_congealed_gooAI@');assert sites(r,'?HasAura@Unit@@')and sites(r,'?DoCast@UnitAI@@')and 201495 in values(r),'accepted aura gates repeats;failed casts retain native retry'
r=ins('?DamageTakenAfterScaling@npc_fallen_crusaderAI@');assert op(r,'mov','byte ptr [rdi + 0x38], 1')and 115163 in values(r),'one native fake death'
r=ins('?DamageTakenAfterScaling@npc_collapsing_starAI@');assert op(r,'cmp','byte ptr [rcx + 0x20], 0')and op(r,'mov','byte ptr [rdi + 0x20], 1')and sites(r,'?GetSummoner@TempSummon@@')and op(r,'cmp','byte ptr [rax + 0x16], 3')and {32953,1000}<=values(r),'pending star protected and native creature summoner/hole/1s cleanup preserved'
print('PASS actual Release twentythird ten:ten final target-relative lethal guards;Aggramar remnants,Argus once pulse and pre-cast radius snapshot/unit-owned100ms immutable callback,Eonar wipe,Worldbreaker both weapons,Hyrja enabled creature owner/restored spirit,Ichor explosion,Desolate actor-owned5s cleanup,Festerface retry,Crusader fake death,star pending/summoner protection. Prior227 selected paths retained,total237;client/full encounter checks remain separate.')
