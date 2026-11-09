"""Verify linked health widths, bounded threshold gates and retry admission."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_eleventh_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
def rows(cls,method):return ins('?'+method+'@'+cls+'@')
for cls,method,count in [('boss_hadronoxAI','KilledUnit',1),('aqsentinelAI','JustDied',3)]:
 r=rows(cls,method);calls=sites(r,'?ModifyHealth@Unit@@');assert len(calls)==count
 for at in calls:assert any(x.mnemonic=='cvttss2si'and x.op_str.startswith('rdx,')for x in r[max(0,at-12):at]),(cls,'signed64 argument')
for method in ['Reset','MoveInLineOfSight']:
 r=rows('boss_valithria_dreamwalkerAI',method);pct,=sites(r,'?CountPctFromMaxHealth@Unit@@');health,=sites(r,'?SetHealth@Unit@@');assert pct<health and op(r[max(0,pct-5):pct],'mov','edx, 0x32') and op(r[pct:health],'mov','rdx, rax')
r=rows('mob_fireswornAI','DamageTakenAfterScaling');assert sites(r,'?GetHealth@Unit@@')and sites(r,'?GetMaxHealth@Unit@@');assert op(r,'cmp','rcx, rax')and op(r,'sub','rax, rcx')and op(r,'cmp','rax, rdi');assert 19497 in values(r)
r=rows('boss_priestess_delrissaAI','UpdateAI');assert op(r,'cmp','rax, r12')and op(r,'mov','r12, rax')and op(r,'mov','r13, rdi'),'64bit running minimum and target update';assert 15000 in values(r)and 17843 in values(r)
r=rows('spell_chimaeron_finkles_mixture_AuraScript','Absorb');assert op(r,'cmp','rax, 0x2710')and op(r,'cmp','rcx, rax')and op(r,'sub','rcx, rax');cmp=next(i for i,x in enumerate(r)if x.mnemonic=='cmp'and x.op_str=='rcx, rax');assert r[cmp+1].mnemonic=='jb','nonlethal excess rejected before subtraction'
r=rows('boss_council_of_eldersAI','PassengerBoarded');clamp=next(i for i,x in enumerate(r)if x.mnemonic=='maxss');cast=next(i for i,x in enumerate(r)if x.mnemonic=='cvttss2si');assert clamp<cast
r=rows('boss_bd_nefarianAI','DamageTakenAfterScaling');assert len(sites(r,'?GetHealth@Unit@@'))==1 and len(sites(r,'?GetMaxHealth@Unit@@'))==2;assert op(r,'cmovae','rcx, rdx')and any(x.mnemonic=='mulsd'for x in r)and any(x.mnemonic=='divsd'for x in r);assert 81272 in values(r)
r=rows('spell_gen_shadowy_reflection','OnProc_0');cmp=next(i for i,x in enumerate(r)if x.mnemonic=='cmp'and x.op_str=='rbx, rax');assert r[cmp+1].mnemonic=='jae';assert op(r,'sub','rbx, rbp');assert ref(rows('spell_gen_shadowy_reflection','Register'),n['symbol']('?OnProc_0@spell_gen_shadowy_reflection@@'))
r=rows('boss_valithria_dreamwalkerAI','MoveInLineOfSight');summon,=sites(r,'?SummonCreature@WorldObject@@');talk,=sites(r,'?Talk@CreatureAI@@');assert summon<talk;assert op(r[summon+1:summon+5],'test','rax, rax')and r[summon+3].mnemonic=='je';assert r[summon+3].operands[0].imm>r[talk].address,'failed summon exits before intro/timers';assert op(r[:summon],'cmp','byte ptr [rcx + 0x19e], 0')and 20000 in values(r)
print('PASS actual Release twelfth ten:two signed64 heals,two full-width half-health checkpoints,post-scaling Firesworn/Nefarian callbacks with target-relative health,Delrissa64 running minimum,Finkle nonlethal guard,Council clamp,Reflection lethal branch,and Valithria summon guard before intro/timers. Prior117 selected paths retained;native/source/client scopes remain distinct.')
