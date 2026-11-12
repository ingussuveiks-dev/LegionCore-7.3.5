from linked_player_abi import player_offset,player_operand
"""Selected x64 executable paths; source/ASAN/native behavior evidence is separate."""
from pathlib import Path
import re,runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions'];symbols=n['symbols'];resolve=n['resolve']
def sites(rows,prefix):
 targets={resolve(v) for k,v in symbols.items() if k.startswith(prefix)}
 return [i for i,x in enumerate(rows) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and resolve(x.operands[0].imm) in targets]
def refs(rows,target):
 return any(o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP and resolve(x.address+x.size+o.mem.disp)==target for x in rows for o in x.operands if x.mnemonic=='lea')
def lambda_body(cls):
 names=[k for k in symbols if k.startswith('?_Do_call@') and cls+'@@' in k];assert len(names)==1,(cls,names)
 return ins(names[0])
for cls,method in [('spell_gen_touch_of_elune','OnUpdate'),('spell_item_leystone_hoofplates','OnUpdate'),('spell_item_demonsteel_stirrups','OnUpdate'),('spell_item_golem','OnUpdate'),('spell_mount_summon_chauffeur','OnApply'),('spell_flight_masters_whistle_argus','HandleAfterCast'),('spell_masters_hunter_seeking_crystal_teleport','HandleAfterCast')]:
 assert refs(ins('?Register@'+cls+'@@'),n['symbol']('?'+method+'@'+cls+'@@')),('Native registered callback missing',cls)
body=ins('?UpdateStatAura@spell_gen_touch_of_elune@@');assert sites(body,'?GetUnitOwner@AuraScript@@') and not sites(body,'?GetTarget@AuraScript@@')
assert sites(ins('?OnUpdate@spell_gen_touch_of_elune@@'),'?UpdateStatAura@spell_gen_touch_of_elune@@')
for cls in ('spell_item_leystone_hoofplates','spell_item_demonsteel_stirrups'):
 body=ins('?OnUpdate@'+cls+'@@');assert sites(body,'?CastSpell@Unit@@')
 assert any(x.mnemonic=='mov' and x.op_str.endswith(', 0x7d0') and 'word ptr' in x.op_str for x in body)
 decrement=next(i for i,x in enumerate(body) if x.mnemonic=='sub' and x.op_str=='r8w, si');assert body[decrement+1].mnemonic=='mov' and 'word ptr' in body[decrement+1].op_str;assert body[decrement-1].mnemonic=='jbe' and body[decrement-2].op_str=='r8d, esi';reset=next(x for x in body if x.address==body[decrement-1].operands[0].imm);assert reset.mnemonic=='mov' and reset.op_str.endswith(', 0x7d0')
body=ins('?ReleaseInteraction@spell_item_golem@@');assert sites(body,'?HasAuraType@Unit@@') and sites(body,'?RemoveFlag@Object@@')
assert sites(ins('?OnRemove@spell_item_golem@@'),'?ReleaseInteraction@spell_item_golem@@')
assert refs(ins('?Register@spell_item_golem@@'),n['symbol']('?OnRemove@spell_item_golem@@'))
body=lambda_body('spell_mount_summon_chauffeur');assert sites(body,'?SummonCreature@WorldObject@@')
assert any(x.mnemonic=='cmp' and x.op_str=='byte ptr [rax], 0' for x in body[:16]),'Old callback still lacks attempt cancellation'
remove=ins('?OnRemove@spell_mount_summon_chauffeur@@');assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rax], 0' for x in remove)
assert sites(remove,'?DespawnOrUnsummon@Creature@@') and not sites(remove,'?RemoveVehicleKit@Unit@@')
for cls in ('spell_flight_masters_whistle_argus','spell_masters_hunter_seeking_crystal_teleport'):
 body=lambda_body(cls);assert sites(body,'?IsInWorld@Object@@') and sites(body,'?IsPreDelete@Object@@') and sites(body,'?NearTeleportTo@Unit@@')
 for offset in (0xa18,0xe0,0x3b8,player_offset(0x6848),player_offset(0x6849),player_offset(0x6851)):
  assert any(x.mnemonic=='cmp' and any(o.type==CS_OP_MEM and o.mem.disp==offset for o in x.operands) for x in body),(cls,hex(offset))
body=ins('?Execute@VehicleJoinEvent@@');lookup,=sites(body,'?GetUnit@ObjectAccessor@@');prefix=body[:lookup]
predelete,=sites(prefix,'?IsPreDelete@Object@@');death=next(i for i,x in enumerate(prefix) if x.mnemonic=='cmp' and x.op_str=='dword ptr [rcx + 0xa18], 0')
assert predelete<death<lookup and prefix[predelete+2].mnemonic=='jne' and prefix[death+1].mnemonic=='jne'
assert prefix[predelete+2].operands[0].imm==prefix[death+1].operands[0].imm,'Death and predelete must use native abort branch'
stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',(n['args'].release/'worldserver.map').read_text(errors='replace'));assert stamp and int(stamp[1],16)==struct.unpack_from('<I',n['binary'],n['pe']+8)[0]
print('PASS selected Release seven-feature paths:real registrations,owner Elune getter,both bounded timers,golem remove/provider check,chauffeur shared-attempt cancellation,origin/instance/life/pending-teleport guards in both actual queued callbacks,native pre-lookup dead-join abort;full behavior separately source-ASAN/native,client pending')
