"""Focused linked width proof; complete historical chain is a separate run."""
from pathlib import Path
import runpy
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM
from native_heal_pe import verify
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions']
def sites(rows,prefix):
    address=n['symbol'](prefix)
    return [i for i,x in enumerate(rows)if x.mnemonic in ('call','jmp')and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==address]
def values(rows):
    return [v for x in rows for operand in x.operands for v in ([operand.imm]if operand.type==CS_OP_IMM else [operand.mem.disp]if operand.type==CS_OP_MEM else [])]
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
verify(ins,sites,values,decoder,n)
rows=ins('?SendHealSpellLog@Unit@@')
assert 0x7fffffff in values(rows)and sum(x.mnemonic=='cmova'for x in rows)>=3,'Three signed32packet fields must saturate only at serialization'
effect=ins('?EffectHeal@Spell@@')
assert sites(effect,'?SpellHealingBonusTaken64@Unit@@')and sites(effect,'?SpellHealingBonusTaken@Unit@@'),'Exact and ordinary native taken-modifier paths both retained'
hit=ins('?DoAllEffectOnTarget@Spell@@IEAAXV?$shared_ptr@UTargetInfo@@@std@@@Z')
assert sites(hit,'?SpellCriticalHealingBonus64@Unit@@')and sites(hit,'?SpellCriticalHealingBonus@Unit@@')and sites(hit,'?HealBySpell@Unit@@'),'Native crit and final healing pipeline retained for both amount channels'
loop=ins('?Absorb@spell_ultraxion_time_loop_AuraScript@')
for m,s in [('mov','rcx, qword ptr [rbx + 0x10]'),('cmp','rcx, rax'),('mov','qword ptr [rsp + 0x40], rax'),('mov','qword ptr [rdx + 8], rax')]:assert any(x.mnemonic==m and x.op_str==s for x in loop),(m,s)
assert 105992 in values(loop)and sites(loop,'?CastCustomSpell@Unit@@QEAAXIAEBVCustomSpellValues@@'),'TimeLoop uses the actual exact-value custom cast'
print('PASS linked full-heal channel,unchanged ordinary modifier/critical paths,TimeLoop exact max-health payload and protocol-only signed32projection. Complete prior chain and real runtime tests remain separate.')
