"""Selected actual native health paths; client encounters remain separate."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_eighth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
def body(cls,method):return ins('?'+method+'@'+cls+'@')
def registered(cls,method):
 rows=body(cls,method);assert ref(body(cls,'Register'),n['symbol']('?'+method+'@'+cls+'@'));return rows
rows=registered('spell_pal_divine_intervention_AuraScript','Absorb')
assert op(rows,'cmp','rax, rbx') and op(rows,'sub','rbx, rax') and op(rows,'cvtsi2ss','xmm0, rbx')
at=next(i for i,x in enumerate(rows)if x.mnemonic=='sub'and x.op_str=='rbx, rax');assert any(x.mnemonic=='jae' and x.operands[0].imm>rows[at].address for x in rows[max(0,at-10):at]),'positive gap before subtraction'
rows=registered('spell_dk_frost_shield_AuraScript','CalculateAmount');assert op(rows,'cvtsi2ss','xmm1, rax') and not any(x.mnemonic=='cvtsi2ss'and x.op_str.endswith(', eax')for x in rows)
rows=registered('spell_stinky_precious_decimate_SpellScript','HandleScript');at,=sites(rows,'?SetHealth@Unit@@');assert op(rows[:at],'imul','rdi, rbx') and op(rows[at-4:at],'mov','rdx, rdi')
rows=body('npc_ember_of_alar','DamageTaken');assert op(rows,'cmp','rbp, rcx') and op(rows,'cmova','rsi, rax') and op(rows,'mov','rdx, rsi')
rows=body('boss_garrosh_hellscreamAI','UpdateAI');copies=[]
for at in sites(rows,'?SetHealth@Unit@@'):
 window=rows[max(0,at-4):at]
 if op(window,'mov','rdx, rbx')or op(window,'mov','rdx, rdi'):copies.append(at)
assert len(copies)==3,'three full-width realm/final clone health caches'
at,=sites(rows,'?ModifyHealth@Unit@@');assert op(rows[max(0,at-6):at],'mov','rdx, rax') and not any(x.mnemonic=='movsxd'for x in rows[max(0,at-6):at]),'60percent heal passed as64bit'
rows=body('boss_immerseusAI','DoAction');assert op(rows,'cmp','rax, rcx') and op(rows,'sub','rax, rcx') and op(rows,'mov','qword ptr [rbx + 0x178], rax');at=next(i for i,x in enumerate(rows)if x.mnemonic=='cmp'and x.op_str=='rax, rcx');assert rows[at+1].mnemonic=='jbe','native floor selected before unsigned subtraction'
rows=registered('spell_ancient_miasma_AuraScript','AfterAbsorb');assert any(x.mnemonic=='addss' for x in rows);names=[name for name in n['symbols']if name.startswith('?CastCustomSpell@Unit@@QEAAXPEAV1@IPEBM11_N')];assert len(names)==1 and sites(rows,names[0]),'native float-pointer barrier visual cast'
rows=registered('spell_deathwhisper_mana_barrier_AuraScript','HandlePeriodicTick');assert op(rows,'sub','rbx, rax') and op(rows,'mov','rdx, rbx') and op(rows,'cmp','rcx, rbx') and op(rows,'cmovl','ebx, eax') and sites(rows,'?ModifyPower@Unit@@')
rows=body('AuraEffect','HandleAuraProcOnHpBelow');assert sum(op([x],'cmp','r13, qword ptr [rsp + 0x68]')for x in rows)==2
assert b'curCount %llu curHealth %llu' in n['binary']
print('PASS actual Release ninth ten:positive Divine gap,Frost64cap,Decimate64destination,Alar64floor,three Garrosh64copies and independent60percent heal,Immerseus64checkpoint/before-subtraction floor,Miasma float amounts and pointer cast,ManaBarrier64heal/bounded signed mana,two native468comparisons/64bit diagnostic. Prior87 paths retained;native/source/client scopes distinct.')
