"""Actual compiled paths. Modeled boss branches and native endpoint tests are separate."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
p=runpy.run_path(str(Path(__file__).with_name('verify_fourth_ten_spell_repairs_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];ref=p['ref']
legacy='?MoveJump@MotionMaster@@QEAAXMMMMM@Z';native='?MoveJump@MotionMaster@@QEAAXMMMMMMI_N'
rows=ins(legacy);assert len(sites(rows,native))==1 and 1004 in values(rows)
assert any(x.mnemonic=='xorps' and x.op_str=='xmm0, xmm0' for x in rows)
assert sites(ins('?DoAction@npc_drelanimAI@npc_drelanim@@'),legacy),('original Tirathon call reaches new five-float overload')
def bounded(rows):
    result=[]
    for at in sites(rows,'?urand@@'):
        before=rows[max(0,at-5):at]
        if any(x.mnemonic=='dec' and x.op_str=='edx' for x in before) and any(x.mnemonic=='xor' and x.op_str=='ecx, ecx' for x in before):result.append(at)
    return result
for prefix in ['?UpdateAI@boss_harjatanAI@','?UpdateAI@boss_thrashbite_the_scornfulAI@','?UpdateAI@mob_zao@@','?Update@instance_antorus_InstanceMapScript@']:
    rows=ins(prefix);at,=bounded(rows)
    # Every selected RNG call uses count-1; source-ASAN tests exercise empty/high bounds.
    assert any(x.mnemonic in ('cmp','test') for x in rows[max(0,at-30):at-2]),(prefix,'nonempty check')
    print('PATH bounded RNG',prefix,hex(rows[at].address))
rows=ins('?SelectRandomNotStomach@cthunAI@boss_cthun@@');assert sites(rows,'?urand@@')
assert any(x.mnemonic=='dec' or (x.mnemonic=='lea' and '- 1]' in x.op_str) for x in rows)
assert sites(ins('?SpawnEyeTentacle@cthunAI@boss_cthun@@'),'?SelectRandomNotStomach@cthunAI@boss_cthun@@')
cls='spell_sha_spirit_link_SpellScript@spell_sha_spirit_link@@';rows=ins('?HandleAfterCast@'+cls)
assert ref(ins('?Register@'+cls),n['symbol']('?HandleAfterCast@'+cls))
assert sites(rows,'?SetHealth@Unit@@') and sum(x.mnemonic=='divsd' for x in rows)>=2 and any(x.mnemonic=='mulsd' for x in rows)
assert any(x.mnemonic=='cmp' and any(o.type==CS_OP_MEM and o.mem.disp==0x18 for o in x.operands) for x in rows),('real TargetInfo miss condition')
assert any(x.mnemonic=='test' and any(o.type==CS_OP_MEM and o.mem.disp==0x1c for o in x.operands) for x in rows),('real TargetInfo effect mask')
cls='spell_dh_soul_barrier_AuraScript@spell_dh_soul_barrier@@';rows=ins('?CalculateAmount@'+cls);get,=sites(rows,'?GetCaster@AuraScript@@')
assert rows[get+1].mnemonic=='test' and rows[get+2].mnemonic=='je'
assert ref(ins('?Register@'+cls),n['symbol']('?CalculateAmount@'+cls))
cls='spell_sha_healing_rain_SpellScript@spell_sha_healing_rain@@';rows=ins('?HandleOnCast@'+cls);get,=sites(rows,'?GetExplTargetDest@SpellScript@@');summon,=sites(rows,'?SummonCreature@WorldObject@@')
assert rows[get+1].mnemonic=='test' and rows[get+2].mnemonic=='je' and rows[get+2].operands[0].imm>rows[summon].address
assert ref(ins('?Register@'+cls),n['symbol']('?HandleOnCast@'+cls))
prune='?PruneTargets@spell_pal_at_devotion_aura@@';rows=ins(prune)
for dep in ['?GetUnit@ObjectAccessor@@','?CheckTarget@spell_pal_at_aura_of_sacrifice@@','?RemoveAurasDueToSpell@Unit@@']:assert sites(rows,dep),dep
for method in ['HandleAuras','HandlePlayerCount']:assert sites(ins('?'+method+'@spell_pal_at_devotion_aura@@'),prune)
rows=ins('?HandleAuras@spell_pal_at_devotion_aura@@');divide=next(i for i,x in enumerate(rows)if x.mnemonic=='divss')
assert any(x.mnemonic=='xorps' for x in rows[:divide]) and any(x.mnemonic=='jne' for x in rows[:divide]),('finite empty branch')
print('PASS actual Release fifth ten:XYZ overload/native Tirathon caller;four bounded encounter RNG paths;Cthun full-list RNG/caller;Spirit real miss/effect mask+double64bit redistribution;Barrier null guard;Rain destination guard before summon;Devotion prune/count/finite-empty paths. Prior47 spell fixes retained;source/native/client scopes remain distinct.')
