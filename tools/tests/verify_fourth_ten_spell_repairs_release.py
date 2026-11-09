"""Verify actual registered compiled paths; behavior/native/client scopes are separate."""
from pathlib import Path
import json,runpy,struct,re
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_third_ten_spell_repairs_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def body(cls,method):
    name='?'+method+'@'+cls;rows=ins(name)
    assert ref(ins('?Register@'+cls),n['symbol'](name)),(cls,method,'registration')
    return rows
def owner(rows):
    assert sites(rows,'?GetUnitOwner@AuraScript@@') and not sites(rows,'?GetTarget@AuraScript@@')
def constants(rows):
    found=[]
    for x in rows:
        for o in x.operands:
            if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
                try:found.append(struct.unpack_from('<f',n['binary'],n['offset'](x.address+x.size+o.mem.disp))[0])
                except (ValueError,struct.error,AssertionError):pass
    return found
rows=body('aura_commanders_valor_location@@','Update');owner(rows)
assert sites(rows,'?Allowed@CommandersValor@@') and sites(rows,'?Remove@AuraScript@@')
rows=body('spell_tirathon_beam_dmg_AuraScript@spell_tirathon_beam_dmg@@','OnUpdate');owner(rows)
assert sites(rows,'?GetCaster@AuraScript@@') and sites(rows,'?CastSpell@Unit@@') and {500,202862}<=values(rows)
rows=body('spell_gulp_frog_toxinAuraScript@spell_gulp_frog_toxin@@','OnStackChange')
emit,=sites(rows,'?CastSpell@Unit@@');assert {10,147656}<=values(rows)
stack,=sites(rows,'?GetStackAmount@AuraScript@@');assert any(x.mnemonic=='jb' for x in rows[stack:emit])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x360], 1' for x in rows[:emit])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x360], 0' for x in rows[emit:])
for raid in json.loads(Path(__file__).with_name('FourthTenVantus.json').read_text()):
    cls=raid['nested']+'@'+raid['cls']+'@@';rows=body(cls,'OnUpdate');owner(rows)
    assert {1000,raid['map']}<=values(rows)
    for dep in ['?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetInstanceScript@WorldObject@@','?GetBossState@InstanceScript@@','?HasAura@Unit@@','?CastSpell@Unit@@','?RemoveAurasDueToSpell@Unit@@']:
        assert sites(rows,dep),(raid['key'],dep)
    emit,=sites(rows,'?CastSpell@Unit@@')
    assert any(x.mnemonic=='mov' and any(o.type==CS_OP_IMM and o.imm==1000 for o in x.operands) for x in rows[:emit])
    assert any(x.mnemonic=='cmp' and any(o.type==CS_OP_MEM and o.mem.disp==0xe0 for o in x.operands) for x in rows[:emit])
    # Read the actual inlined table referenced by the callback; verify all40 typed IDs.
    candidates=[]
    for x in rows:
        if x.mnemonic=='lea':
            for o in x.operands:
                if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
                    at=n['offset'](x.address+x.size+o.mem.disp)
                    actual=[struct.unpack_from('<III',n['binary'],at+12*i) for i in range(len(raid['table']))]
                    if [(r,h) for r,h,b in actual]==[(e['rune'],e['helper']) for e in raid['table']]:candidates.append(actual)
    assert len(candidates)==1,(raid['key'],'actual compiled table')
    enum=re.sub(r'//[^\n]*','',raid['enum']);items=enum[enum.index('{')+1:enum.rindex('}')].split(',');mapping={};number=-1
    for item in items:
        item=item.strip()
        if not item:continue
        if '=' in item:key,val=item.split('=',1);number=int(val.strip(),0)
        else:key=item;number+=1
        mapping[key.strip()]=number
    assert [r[2] for r in candidates[0]]==[mapping[e['boss'].split('::')[-1]] for e in raid['table']]
    remove=body(cls,'OnRemove');owner(remove);assert sites(remove,'?RemoveAurasDueToSpell@Unit@@')
rows=body('spell_pri_void_shift_SpellScript@spell_pri_void_shift@@','HandleDummy')
assert len(sites(rows,'?GetHealthPct@Unit@@'))==2 and len(sites(rows,'?CastCustomSpell@Unit@@'))==2
assert sum(x.mnemonic=='maxss' for x in rows)==2 and sum(x.mnemonic=='mulss' for x in rows)>=2
assert not sites(rows,'?CountPctFromMaxHealth@Unit@@')
# The native64-bit health inputs are converted only after percentage multiplication.
assert sum(x.mnemonic=='cvtsi2ss' and x.op_str.endswith(', rax') for x in rows)>=4
for method in ['Load','CalculateAmount']:
    cls='spell_dru_swift_flight_passive_AuraScript@spell_dru_swift_flight_passive@@'
    rows=ins('?'+method+'@'+cls) if method=='Load' else body(cls,method);owner(rows)
    assert not sites(rows,'?GetCaster@AuraScript@@')
assert {375,struct.unpack('<I',struct.pack('<f',310))[0]}<=values(rows)
for cls in ['spell_dk_anti_magic_zone_AuraScript@spell_dk_anti_magic_zone@@','spell_gen_artificial_stamina_AuraScript@spell_gen_artificial_stamina@@','spell_gen_artificial_threat@@']:
    rows=body(cls,'CalculateAmount');get,=sites(rows,'?GetCaster@AuraScript@@')
    assert rows[get+1].mnemonic=='test' and rows[get+2].mnemonic=='je',(cls,'null guard')
    assert not sites(rows,'?GetUnitOwner@AuraScript@@'),(cls,'retain original caster stats')
for method in ['HandleEffectApply','HandlePeriodicTick']:owner(body('spell_censer_of_eternal_agony@@',method))
rows=body('spell_item_rancid_maw@@','HandleDamage')
assert sites(rows,'?GetDistance@WorldObject@@') and sites(rows,'?SetHitDamage@SpellScript@@')
assert 20.0 in constants(rows) and all(any(x.mnemonic==op for x in rows) for op in ['minss','divss','mulss'])
print('PASS actual Release fourth ten:registered owner/guard/stack/calc/distance paths;five inlined Vantus callbacks/40typed pairs/1000ms rearm/native map+state+cleanup;prior37 spell paths retained. Source-ASAN/native/client evidence separate.')
