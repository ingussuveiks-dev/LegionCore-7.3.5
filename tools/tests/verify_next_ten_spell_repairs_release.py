"""Require the selected compiled callbacks and their real hook registrations."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
prior=runpy.run_path(str(Path(__file__).with_name('verify_ten_spell_lifecycles_release.py')))
n=prior['n'];ins=n['instructions'];symbols=n['symbols'];resolve=n['resolve'];sites=prior['sites'];values=prior['values'];ref=prior['ref']
for cls,method,gate in [('spell_gen_umbral_shift@@','CalculateMastery',253825),('spell_warr_shatter_the_bones@@','CalculateAmount',132404),('spell_item_chocolate_cookie_AuraScript@spell_item_chocolate_cookie@@','OnStackChange',91)]:
 prefix='?'+method+'@'+cls;body=ins(prefix)
 assert sites(body,'?GetUnitOwner@AuraScript@@')and not sites(body,'?GetTarget@AuraScript@@')and not sites(body,'?GetCaster@AuraScript@@'),(cls,'owner')
 assert gate in values(body),(cls,'authored gate')
 assert ref(ins('?Register@'+cls),n['symbol'](prefix)),(cls,'registration')
body=ins('?OnStackChange@spell_item_chocolate_cookie_AuraScript@spell_item_chocolate_cookie@@')
assert 99041 in values(body)and sites(body,'?UpdateAchievementCriteria@Player@@')
for cls,count,endpoint in [('spell_mage_comet_storm',1,'?CastSpell@Unit@@'),('spell_mage_flurry',2,'?CastCustomSpell@Unit@@'),('spell_storm_comet',1,'?SummonCreature@WorldObject@@'),('spell_gen_reckless_stomp',1,'?CastSpell@Unit@@'),('spell_warl_unstable_affliction',1,'?CastCustomSpell@Unit@@')]:
 names=[k for k in symbols if k.startswith('?_Do_call@')and cls+'@@'in k];assert len(names)==count,(cls,len(names))
 for name in names:
  body=ins(name)
  for dep in ('?IsInWorld@Object@@','?IsPreDelete@Object@@'):assert sites(body,dep),(cls,dep)
  emits=sites(body,endpoint);assert emits,(cls,endpoint)
  for off in (0xe0,0x3b8):
   checks=[i for i,x in enumerate(body[:emits[0]])if x.mnemonic=='cmp'and any(o.type==CS_OP_MEM and o.mem.disp==off for o in x.operands)]
   assert len(checks)==1,(cls,hex(off))
   branch=body[checks[0]+1];assert branch.mnemonic=='jne'and branch.operands[0].imm>body[emits[-1]].address,(cls,'must skip emission')
  assert not any(x.mnemonic=='cmp'and any(o.type==CS_OP_MEM and o.mem.disp==0xa18 for o in x.operands)for x in body),'No added death discard'
for cls,method in [('spell_mage_comet_storm@@','HandleDummy'),('spell_mage_flurry_SpellScript@spell_mage_flurry@@','HandleLaunchTarget'),('spell_storm_comet@@','HandleDummy'),('spell_gen_reckless_stomp_SpellScript@spell_gen_reckless_stomp@@','HandleDummy'),('spell_warl_unstable_affliction_AuraScript@spell_warl_unstable_affliction@@','HandleDispel')]:
 assert ref(ins('?Register@'+cls),n['symbol']('?'+method+'@'+cls)),(cls,'real hook registration')
body=ins('?Trigger@spell_item_walling_souls@@');division=next(i for i,x in enumerate(body)if x.mnemonic=='divss');cd,=sites(body,'?ModifySpellCooldown@Player@@')
load=next(x for x in body[:division]if x.mnemonic=='movss'and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP)
address=load.address+load.size+load.operands[1].mem.disp
assert struct.unpack_from('<f',n['binary'],n['offset'](address))[0]==1.0
assert any(x.mnemonic=='comiss'for x in body[:division])and any(x.mnemonic=='ja'and x.operands[0].imm>body[cd].address for x in body[:division])
assert ref(ins('?Register@spell_item_walling_souls@@'),n['symbol']('?Trigger@spell_item_walling_souls@@'))
body=ins('?HandleDummy@spell_item_flask_of_the_north_SpellScript@spell_item_flask_of_the_north@@')
assert sites(body,'?GetCaster@SpellScript@@')and sites(body,'?irand@@')
table=next(o.mem.disp for x in body for o in x.operands if x.mnemonic=='mov'and o.type==CS_OP_MEM and o.mem.scale==4 and o.mem.index)
entries=struct.unpack_from('<12I',n['binary'],n['offset'](n['base']+table))
assert entries[9]==entries[6]and entries[11]==entries[2],('Monk/Shaman and DH/Hunter class choices',entries)
assert 11 in values(body)and any(x.mnemonic=='ja'for x in body),'Unknown class must exit switch'
print('PASS actual Release ten repairs:3 owner hooks/gates;6 registered delayed closures with original map/instance/world guards before emission;Wailing1.0 guard before division/cooldown;Flask12-class table with Monk/DH paths. Prior17 spell paths retained. Behavior native/source-ASAN and client checks separate.')
