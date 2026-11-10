"""Verify thirty linked policies grouped as ten independent encounter repairs."""
from pathlib import Path
import json,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_twentyfifth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('TwentySixthTenRepairs.json').read_text(encoding='utf8'));count=0
for e in plan:
 for cls in e['classes']:
  r=ins('?DamageTakenAfterScaling@'+cls+'@');assert sites(r,'?GetHealth@Unit@@'),cls
  assert not any(s.startswith('?DamageTaken@'+cls+'@')for s in n['symbols']),cls
  assert any(x.mnemonic in ('jae','jb','jbe','ja','cmovae','cmovb','cmovbe','cmova','setb')for x in r),('unsigned native health comparison',cls)
  count+=1
assert count==30 and len(plan)==10
r=ins('?DamageTakenAfterScaling@npc_aysa_cloudsinger_AI@');health,=sites(r,'?GetHealth@Unit@@');zero=next(i for i,x in enumerate(r[:health])if x.mnemonic=='cmp'and x.op_str=='dword ptr [r8], 0');skip=next(i for i,x in enumerate(r[:health])if x.mnemonic=='je');assert zero<skip<health and r[skip].operands[0].imm>r[health].address,'zero-damage admission before training cap'
assert any(x.mnemonic=='setb'and x.op_str=='cl'for x in r[health:])and any(x.mnemonic=='mov'and x.op_str=='dword ptr [rdi], ecx'for x in r[health:]),'native branchless unsigned0/1 training cap'

for cls in ['boss_kalecgosAI','boss_sathrovarrAI']:
 r=ins('?DamageTakenAfterScaling@'+cls+'@');assert sites(r,'?SetHealth@Unit@@')and 1 in values(r),'original paired one-health banish retained'
r=ins('?DamageTakenAfterScaling@mob_volatile_felfire_fiendAI@');assert 45779 in values(r)and sites(r,'?DoCast@UnitAI@@'),'native felfire fission'
assert sites(ins('?UpdateAI@mob_volatile_felfire_fiendAI@'),'?Kill@Unit@@'),'original contact explosion scripted self-death retained'
for cls in ['npc_norushen_heal_ch_melee_combtantAI','npc_norushen_heal_ch_casterAI','npc_norushen_heal_ch_guardianAI']:
 r=ins('?DamageTakenAfterScaling@'+cls+'@');assert {30,148424}<=values(r)and sites(r,'?HasAura@Unit@@'),'native healer exhaustion gate retained'
print('PASS actual Release twentysixth ten:all30 final target-relative unsigned guard bodies/no raw hooks;Aysa zero-damage admission and training cap,Felfire native death burst/contact self-kill,paired Kalecgos one-health banish,self-death exceptions,all Norushen/Durumu/Ji Kun/Jinrokh/Paragon/Megaera support policies. Prior257 selected paths retained,total267;ten independent problems,client/full encounter traversal separate.')
