"""Linked timer gates, actor resolution, original-instance guards and ride control."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_eighteenth_ten_release.py')));p=p['p']|p
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded'];map_guards=p['map_guards']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
r=ins('?UpdateAI@npc_mimirons_infernoAI@');at=next(i for i,x in enumerate(r)if x.mnemonic=='cmp'and x.op_str=='byte ptr [rcx + 0x38], 0');assert r[at+3].mnemonic=='je'and op(r,'mov','eax, dword ptr [rcx + 0x3c]'),'inactive pause gates timer read'
assert 60000 in values(ins('?GetAI@npc_hodirs_fury@@')),'inlined constructor native timer initialization'
r=ins('?Reset@boss_ymironAI@');assert any(x.mnemonic=='mov'and x.op_str.endswith(', 0')and 'word ptr'in x.op_str for x in r),'walking/pause reset'
prisoner,=callbacks('npc_ymiron_seacursed_slaver','MoveInLineOfSight');r=expanded(prisoner);assert sites(r,'?GetCreature@ObjectAccessor@@'),'fresh prisoner lookup'
assistants=[expanded(s)for s in callbacks('instance_return_to_karazhan_InstanceMapScript','SetBossState')if sites(expanded(s),'?GetCreature@Map@@')];assert len(assistants)==1,'fresh instance-owned assistant lookup'
brutes=[expanded(s)for s in callbacks('boss_rtk_cogglestonAI','JustDied')if sites(expanded(s),'?GetCreature@ObjectAccessor@@')];assert len(brutes)==1 and sites(brutes[0],'?UpdateEntry@Creature@@'),'fresh Brute before native transformation'
# Nested ApplyOnEveryPlayer has both outer and delayed callables; only delayed one carries captured origin comparisons.
guarded=[]
for s in callbacks('instance_trial_of_valor_InstanceMapScript','SetBossState'):
 r=expanded(s)
 if sites(r,'?CombatStop@Unit@@'):
  try:map_guards(r);guarded.append(r)
  except AssertionError:pass
assert guarded,'Odyn original instance before combat cleanup'
assault=callbacks('instance_AssaultBrokenShore_InstanceMapScript','onScenarionNextStep');assert len(assault)==4
for s in assault:map_guards(expanded(s))
intro=callbacks('instance_broken_islands_InstanceMapScript','OnPlayerEnter');assert len(intro)==3
for s in intro:map_guards(expanded(s))
vetir,=callbacks('npc_invasion_stormheim_vetirAI','WaypointReached');r=expanded(vetir);map_guards(r);assert sites(r,'?TeleportTo@Player@@')and 232864 in values(r)
r=ins('?PassengerBoarded@npc_invasion_stormheim_vetirAI@');assert any(x.mnemonic=='test'and x.op_str=='sil, sil' for x in r)and sites(r,'?SetClientControl@Player@@'),'removal restores actual player mover'
r=ins('?UpdateAI@npc_invasion_stormheim_vetirAI@');assert sites(r,'?GetVehicleBase@Unit@@')and sites(r,'?SetClientControl@Player@@'),'actual live seat required before handoff'
print('PASS actual Release nineteenth ten:active Inferno timers,Hodir60000,Ymiron walking reset;three delayed actor lookups;Odyn/four Assault/three BrokenIntro original live instance guards;Vetir removal branch,seat guard and extraction origin. Prior187 selected paths retained;automated/native/client scopes distinct.')
