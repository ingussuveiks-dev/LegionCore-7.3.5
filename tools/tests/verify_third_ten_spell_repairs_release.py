"""Selected third-batch compiled callback paths;source/native behavior is separate."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
prior=runpy.run_path(str(Path(__file__).with_name('verify_next_ten_spell_repairs_release.py')))
n=prior['n'];ins=prior['ins'];symbols=n['symbols'];sites=prior['sites'];ref=prior['ref'];values=prior['values']
for cls,method in [('spell_item_faded_wizard_hat@@','OnApply'),('spell_gen_citizen_costume_alliance_darkmoon@@','AfterApply'),('spell_gen_citizen_costume_horde_darkmoon@@','AfterApply'),('spell_item_prydaz_abs_AuraScript@spell_item_prydaz_abs@@','CalculateAmount'),('spell_item_ocean_embrace_AuraScript@spell_item_ocean_embrace@@','OnApply'),('spell_item_ocean_embrace_AuraScript@spell_item_ocean_embrace@@','OnRemove'),('spell_pri_shadowform@@','OnRemove')]:
 prefix='?'+method+'@'+cls;body=ins(prefix)
 assert sites(body,'?GetUnitOwner@AuraScript@@')and not sites(body,'?GetCaster@AuraScript@@'),(cls,method,'owner')
 assert ref(ins('?Register@'+cls),n['symbol'](prefix)),(cls,method,'registration')
body=ins('?UpdateVisuals@spell_pri_shadowform@@');assert sites(body,'?GetUnitOwner@AuraScript@@')and not sites(body,'?GetCaster@AuraScript@@')
for method in ['OnApply','OnUpdate']:
 caller=ins('?'+method+'@spell_pri_shadowform@@')
 assert sites(caller,'?UpdateVisuals@spell_pri_shadowform@@')or(sites(caller,'?GetUnitOwner@AuraScript@@')and not sites(caller,'?GetCaster@AuraScript@@')and {185908,185909,185910,185911}<=values(caller)),('inlined native owner/visual helper',method)
 assert ref(ins('?Register@spell_pri_shadowform@@'),n['symbol']('?'+method+'@spell_pri_shadowform@@'))
body=ins('?OnRemove@spell_item_demon_hunters_aspect@@');assert sites(body,'?RestoreDisplayId@Unit@@')and not sites(body,'?SetDisplayId@Unit@@')
assert ref(ins('?Register@spell_item_demon_hunters_aspect@@'),n['symbol']('?OnRemove@spell_item_demon_hunters_aspect@@'))
for cls,count in [('spell_dk_glacial_advance',1),('spell_monk_breath_of_fire',3)]:
 names=[k for k in symbols if k.startswith('?_Do_call@')and cls+'@@'in k];assert len(names)==count,(cls,len(names))
 for name in names:
  body=ins(name)
  if cls=='spell_dk_glacial_advance':
   operator,=[k for k in symbols if k.startswith('??R<lambda_1>@')and cls+'@@'in k]
   assert sites(body,operator),('real adapter-to-operator path',cls)
   body=ins(operator)
  emits=sites(body,'?CastSpell@Unit@@');assert emits,(cls,'native casts')
  for dep in ['?IsInWorld@Object@@','?IsPreDelete@Object@@']:assert sites(body,dep),(cls,dep)
  for off in [0xe0,0x3b8]:
   checks=[i for i,x in enumerate(body[:emits[0]])if x.mnemonic=='cmp'and any(o.type==CS_OP_MEM and o.mem.disp==off for o in x.operands)]
   assert len(checks)==1,(cls,hex(off));branch=body[checks[0]+1]
   assert branch.mnemonic=='jne'and branch.operands[0].imm>body[emits[-1]].address,(cls,'origin guard must skip cast')
 for nested,method in [('spell_dk_glacial_advance_SpellScript@spell_dk_glacial_advance@@','HandleAfterCast')]if cls=='spell_dk_glacial_advance'else[('spell_monk_breath_of_fire@@','HandleOnCast')]:
  assert ref(ins('?Register@'+nested),n['symbol']('?'+method+'@'+nested))
cls='spell_gen_hammer_of_supreme_justice_AuraScript@spell_gen_hammer_of_supreme_justice@@'
load=ins('?Load@'+cls);assert sites(load,'?GetUnitOwner@AuraScript@@')and not sites(load,'?GetCaster@AuraScript@@')
body=ins('?OnUpdate@'+cls);tele,=sites(body,'?NearTeleportTo@Unit@@')
for dep in ['?IsInWorld@Object@@','?IsPreDelete@Object@@']:assert sites(body,dep)
for off in [0xe0,0x3b8]:assert any(x.mnemonic=='cmp'and any(o.type==CS_OP_MEM and o.mem.disp==off for o in x.operands)for x in body[:tele])
assert ref(ins('?Register@'+cls),n['symbol']('?OnUpdate@'+cls))
cls='spell_item_brewfest_mount_transformation_SpellScript@spell_item_brewfest_mount_transformation@@';body=ins('?HandleDummy@'+cls)
remove,=sites(body,'?RemoveAurasByType@Unit@@');cmp=next(i for i,x in enumerate(body[:remove])if x.mnemonic=='comiss')
mem=body[cmp].operands[1];assert mem.type==CS_OP_MEM and mem.mem.base==X86_REG_RIP
address=body[cmp].address+body[cmp].size+mem.mem.disp;assert struct.unpack_from('<f',n['binary'],n['offset'](address))[0]==2.0
assert any(x.mnemonic=='movss'and any(o.type==CS_OP_MEM and o.mem.disp==0x2f18 for o in x.operands)for x in body[:cmp])
assert any(x.mnemonic=='setae'and x.op_str=='dil'for x in body[cmp:remove])and any(x.mnemonic=='test'and x.op_str=='dil, dil'for x in body[remove:])
assert {49357,52845,43900,43899,49379,49378}<=values(body)
assert ref(ins('?Register@'+cls),n['symbol']('?HandleDummy@'+cls))
print('PASS actual Release third ten:7 registered owner hooks plus Shadow helper/2callers;Aspect native RestoreDisplayId;four guarded queued closures;Hammer owner-load/origin/world before teleport;Brewfest speed2.0 captured before dismount and used afterward. Prior27 spell paths retained. Source-ASAN/native/client scopes remain distinct.')
