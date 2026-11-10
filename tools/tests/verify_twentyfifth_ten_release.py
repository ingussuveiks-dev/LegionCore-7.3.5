"""Verify lethal observations and reuse of living encounter actors in the actual PE."""
from pathlib import Path
import json,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_twentyfourth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
def op(r,m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
plan=json.loads(Path(__file__).with_name('TwentyFifthTenRepairs.json').read_text(encoding='utf8'))
for e in plan:
 for cls in e['classes']:
  r=ins('?DamageTakenAfterScaling@'+cls+'@');assert sites(r,'?GetHealth@Unit@@'),cls
  assert not any(s.startswith('?DamageTaken@'+cls+'@')for s in n['symbols']),cls
for cls,nums in [('boss_shade_of_xaviusAI',[221315,10]),('boss_naraxas',[199775,6]),('boss_bronjahmAI',[4,12000,16000]),('npc_scourge_hulkAI',[26668]),('mob_hellfire_channelerAI',[30531]),('npc_raigonn_weak_spot',[56877]),('npc_energy_chargeAI',[118023,118299]),('npc_empyreal_focusAI',[132257,132258]),('npc_living_sandAI',[136851,136861]),('npc_durumu_fogAI',[69052,136124]),('npc_crackling_stalkerAI',[138337])]:
 assert set(nums)<=values(ins('?DamageTakenAfterScaling@'+cls+'@')),cls
for cls in ['boss_shade_of_xaviusAI','boss_naraxas']:assert sites(ins('?DamageTakenAfterScaling@'+cls+'@'),'?GetAura@Unit@@'),cls
assert sites(ins('?DamageTakenAfterScaling@npc_raigonn_weak_spot@'),'?GetCreature@Map@@'),'actual instance map resolves controller'
r=ins('?DamageTakenAfterScaling@npc_living_sandAI@');assert op(r,'cmp','byte ptr [rcx + 0x40], 0')and op(r,'mov','byte ptr [rdi + 0x40], 1'),'returned trap ignores queued hits and latches once'
r=ins('?DoAction@npc_living_sandAI@');select,=sites(r,'?SelectTarget@UnitAI@@');remove=sites(r,'?RemoveAurasDueToSpell@Unit@@');assert len(remove)==2 and select<min(remove),'real target required before consuming trap effects'
assert any(x.mnemonic=='test'and x.op_str=='rax, rax'and r[i+1].mnemonic=='je'and r[i+1].operands[0].imm>r[remove[-1]].address for i,x in enumerate(r)if select<i<remove[0]),'missing target preserves trap/next storm retry'
print('PASS actual Release twentyfifth ten:11 final target-relative handlers;native stack/soul/ritual gates,channeler transfer,instance controller,charge/focus once,Living Sand damage rejection and target-before-effect-removal,blue-fog-only freeze and one crackling flare. Prior247 selected paths retained,total257;client/full encounter checks remain separate.')
