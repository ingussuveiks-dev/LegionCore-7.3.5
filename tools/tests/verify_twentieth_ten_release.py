"""Linked final ten production lifecycles; retains all preceding selected checks."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_nineteenth_ten_release.py')));n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded'];map_guards=p['map_guards']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
r=ins('?UpdateAI@boss_bd_nefarianAI@');assert op(r,'cmp','dword ptr [rdi + 0x178], 2')and op(r,'cmp','dword ptr [rdi + 0x178], 3')
for slot,phase in [('0x180',2),('0x17c',2),('0x184',3)]:
 at=next(i for i,x in enumerate(r)if x.mnemonic=='mov'and x.op_str=='eax, dword ptr [rdi + '+slot+']');assert any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rdi + 0x178], '+str(phase) for x in r[max(0,at-6):at]),'phase guard before timer read '+slot
assert op(ins('?Reset@mob_annhylde_the_callerAI@'),'mov','qword ptr [rcx + 0x50], 0')
r=ins('?UpdateAI@npc_defiant_trollAI@');assert op(r,'mov','dword ptr [rcx + 0x38], 0')and sites(r,'?MoveTargetedHome@MotionMaster@@')
for cls in ['npc_chens','npc_scgnottz','npc_lin_cloudwalker','npc_rungle','npc_zang_cloudwalker','npc_hemet_nesingwary','npc_emi_lan']:
 name,=callbacks(cls,'UpdateAI');r=expanded(name);map_guards(r);assert sites(r,'?GetCreature@ObjectAccessor@@')and op(r,'cmp','qword ptr [rcx + 0x30d8], 0')and sites(r,'?HasAura@Unit@@')and sites(r,'?IsWithinDistInMap@WorldObject@@'),'exact live waiting balloon '+cls
for cls,lookup in [('npc_shannox_crystal_prisonAI','?GetUnit@ObjectAccessor@@'),('npc_maloriak_flash_freezeAI','?GetPlayer@ObjectAccessor@@'),('npc_ice_tombAI','?GetPlayer@ObjectAccessor@@'),('npc_hagara_the_stormbinder_icy_tombAI','?GetPlayer@ObjectAccessor@@')]:
 r=ins('?JustDied@'+cls+'@');assert op(r,'cmp','byte ptr [rcx + 0x48], 0')and op(r,'mov','byte ptr [rcx + 0x48], 1'),'release once '+cls
 at,=sites(r,lookup);clear,=sites(r,'?Clear@ObjectGuid@@');assert clear>at and not any(x.mnemonic.startswith('j')for x in r[at+1:clear]),'clear identity independently of resolution '+cls
 assert op(ins('?SetGUID@'+cls+'@'),'cmp','byte ptr [rcx + 0x48], 0'),'late rebind rejected '+cls
r=ins('?Reset@npc_image_of_medivhAI@');assert op(r,'mov','byte ptr [rcx + 0x60], 0')and op(r,'mov','qword ptr [rcx + 0x58], rbp')and op(r,'mov','qword ptr [rcx + 0x50], rbp')
r=ins('?MovementInform@npc_image_of_medivhAI@');summon,=sites(r,'?SummonCreature@WorldObject@@');abort,=sites(r,'?AbortEvent@npc_image_of_medivhAI@');assert abort>summon and op(r,'mov','byte ptr [rdi + 0x60], 1'),'inlined StartEvent commits successful summon only'
assert sites(ins('?NextStep@npc_image_of_medivhAI@'),'?AbortEvent@npc_image_of_medivhAI@'),'missing/dead partner aborts before later sequence'
r=ins('?UpdateAI@npc_image_of_medivhAI@');assert op(r,'cmp','byte ptr [rcx + 0x60], 0')and op(r,'test','ecx, ecx'),'idle event and unarmed fire timer guarded'
r=ins('?DamageTakenAfterScaling@npc_argent_championAI@');assert sites(r,'?GetAffectingPlayer@Unit@@')and sites(r,'?GetHealth@Unit@@')and op(r,'cmp','rbx, rax')and op(r,'mov','byte ptr [rdi + 0x44], 1')and sites(r,'?RewardPlayerAndGroupAtEvent@Player@@')and 33708 in values(r),'scaled exact lethal player/owner credit once'
assert sites(r,'?CombatStop@Unit@@')and sites(r,'?DeleteThreatList@Unit@@')and not sites(r,'?EnterEvadeMode@'),'cleanup preserves surrender latch'
print('PASS actual Release twentieth ten:three phase timer guards;AnnHylde idle state;single troll return;all7 exact-summon balloon guards;four once-only prison releases/unconditional GUID retirement/late-bind rejection;Medivh successful activation/abort/idle clocks;post-scaling exact-lethal affecting-player/group credit without latch-resetting evade. Prior197 selected paths retained;50 repairs across five batches,client traversal remains separate.')
