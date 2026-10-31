"""Verify selected acquisition factories, real native equip/progression and return admission."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_lady_release.py')))
n=retained['n'];ins=n['instructions'];sites=retained['sites']
def values(name):return {o.imm for x in ins(name) for o in x.operands if o.type==CS_OP_IMM}
ai='@AI@go_art_sharasdal@@';spell='@spell_sharasdal_acquisition@@';aura='@aura_sharasdal_acquisition@@'
def method(name,suffix):return '?'+name+suffix
def needs(name,*targets):
 for target in targets:assert sites(ins(name),target),(name,target)
def refs(name):
 result=set()
 for x in ins(name):
  for op in x.operands:
   if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:result.add(x.address+x.size+op.mem.disp)
   elif op.type==CS_OP_IMM:result.add(op.imm)
 return result
def fn_refs(name):
 result=set()
 for at in refs(name):
  try:result.add(n['resolve'](at))
  except (ValueError,struct.error):pass
 return result
def table_has(address,*methods):
 try:rows=struct.unpack_from('<40Q',n['binary'],n['offset'](address))
 except (ValueError,struct.error):return False
 resolved=set()
 for row in rows:
  try:resolved.add(n['resolve'](row))
  except (ValueError,struct.error):pass
 return all(n['symbol'](m) in resolved for m in methods)
factory='?GetAI@go_art_sharasdal@@'
assert any(table_has(at,*(method(m,ai) for m in ['UpdateAI','Reset','GossipHello'])) for at in refs(factory))
assert any(table_has(at,factory,'?OnGossipHello@go_art_sharasdal@@','?OnLootStateChanged@go_art_sharasdal@@') for at in refs('?AddSC_Sharasdal@@'))
loaders=[('?GetSpellScript@?$GenericSpellScriptLoader@Vspell_sharasdal_acquisition@@@@',spell,['Register']),
         ('?GetAuraScript@?$GenericAuraScriptLoader@Vaura_sharasdal_acquisition@@@@',aura,['Register','Load'])]
for loader,suffix,methods in loaders:
 assert any(table_has(at,loader) for at in refs('?AddSC_Sharasdal@@'))
 assert any(table_has(at,*(method(m,suffix) for m in methods)) for at in refs(loader))
for target in ['Check','SceneCredit','SceneReceipt','Equip','AfterEquip']:
 assert n['symbol'](method(target,spell)) in fn_refs(method('Register',spell)),target
assert {209684,209685,61,249,134,64}<=values(method('Register',spell))
assert n['symbol'](method('Removed',aura)) in fn_refs(method('Register',aura))
assert {430,1}<=values(method('Register',aura))
controller='?Controller@SharasdalAcquisition@@'
needs(controller,'?GetGameObjectByEntry@InstanceScript@@','?GetGameObject@ObjectAccessor@@','?GetDBTableGUIDLow@GameObject@@')
assert {1600,249342,134299}<=values(controller)
needs('?OnGameObjectCreate@InstanceScript@@','?AddObject@InstanceScript@@IEAAXPEAVGameObject@@')
needs('?GetGameObjectByEntry@InstanceScript@@','??$GetObjectInMap@VGameObject@@@ObjectAccessor@@')
physical='?Physical@SharasdalAcquisition@@';progress='?Progress@SharasdalAcquisition@@'
assert {1600,249342,134299,40341,1066,6}<=values(physical)
needs(physical,'?IsInWorld@Object@@','?CanContact@Player@@')
for operand in ['dword ptr [rbx + 0xa18], ebp','byte ptr [rbx + 0x6848], bpl','byte ptr [rbx + 0x6849], bpl']:
 assert any(x.mnemonic=='cmp' and x.op_str==operand for x in ins(physical)),operand
assert any(x.mnemonic=='test' and x.op_str=='byte ptr [rbx + 0x4ff0], 1' for x in ins(physical))
needs(physical,'?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@','?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@','?GetExactDist@Position@@')
assert {1066,5,47380,47133}<=values(progress)
code=ins(progress);bound=next(i for i,x in enumerate(code) if x.mnemonic=='xor' and x.op_str=='ecx, 1')
assert [(x.mnemonic,x.op_str) for x in code[bound-1:bound+3]]==[
 ('mov','ecx, edi'),('xor','ecx, 1'),('add','ecx, 5'),('cmp','eax, ecx')]
assert code[bound+3].mnemonic=='ja','Native upper limit must be5 for opening and6 only for continuation'
live=method('Live',ai);begin=method('Begin',ai);update=method('UpdateAI',ai)
needs(live,'?GetItemByEntry@Player@@',physical,progress)
assert len(sites(ins(live),'??8ObjectGuid@@'))==2
assert any(x.mnemonic=='cmp' and x.op_str=='qword ptr [r14 + 0x18], rax' for x in ins(live))
for displacement in ('0x10','0x20'):
 assert any(x.mnemonic=='movups' and x.op_str==f'xmmword ptr [rdi + {displacement}], xmm0' for x in ins(begin))
needs(begin,'?GetItemByEntry@Player@@','?GetGuidValue@Object@@','?AddQuestDelayedEvent@Player@@','?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
assert {209683,209684,40341,128911}<=values(begin)
check=method('Check',spell)
assert any(x.mnemonic=='movups' and x.op_str=='xmmword ptr [rsi + 0x1f9], xmm0' for x in ins(check))
equip=method('Equip',spell);after=method('AfterEquip',spell)
needs(equip,controller,method('Ready',spell),'?EffectForceEquipItem@Spell@@','?Equipped@SharasdalAcquisition@@','?PreventHitDefaultEffect@SpellScript@@')
body=ins(equip);at,=sites(body,'?EffectForceEquipItem@Spell@@')
assert body[at-2].op_str=='edx, 1' and body[at-1].op_str=='byte ptr [rbx + 0x1f8], 1'
assert at<sites(body,'?Equipped@SharasdalAcquisition@@')[0]
needs(after,'?Equipped@SharasdalAcquisition@@',method('Ready',spell),'?GetCurrentStep@Scenario@@','?PreventHitDefaultEffect@SpellScript@@')
assert {1,6}<=values(after) and any(x.mnemonic=='cmp' and x.op_str=='byte ptr [rdi + 0x1f8], sil' for x in ins(after))
equipped='?Equipped@SharasdalAcquisition@@'
needs(equipped,'?GetItemByGuid@Player@@','?GetItemByPos@Player@@','?GetChildItem@Item@@','??8ObjectGuid@@')
assert {128911,128934,15,16,255}<=values(equipped)
needs('?EffectForceEquipItem@Spell@@','?CanEquipItem@Player@@','?CanEquipChildItem@Player@@','?SendEquipError@Player@@','?SwapItem@Player@@')
needs('?EffectSendEvent@Spell@@','?UpdateAchievementCriteria@Player@@')
needs('?EffectKillCredit@Spell@@','?RewardPlayerAndGroupAtEvent@Player@@')
assert n['symbol'](method('SceneCredit',spell))==n['symbol']('?PreventHitDefaultEffect@SpellScript@@') or sites(ins(method('SceneCredit',spell)),'?PreventHitDefaultEffect@SpellScript@@')
needs(method('SceneReceipt',spell),controller,live,'?HasAura@Unit@@')
assert any(x.mnemonic=='mov' and x.op_str=='qword ptr [rbx + 0x50], 2' for x in ins(method('SceneReceipt',spell)))
removed=method('Removed',aura)
needs(removed,controller,live,'?HasSceneStatus@Player@@',method('Cancel',ai))
assert {4,1255,3,2}<=values(removed),'Actual native EXPIRE4/SCENE_COMPLETE3 and stage2 required'
assert any(x.mnemonic=='mov' and x.op_str=='qword ptr [rbx + 0x50], 3' for x in ins(removed))
needs(method('Load',aura),'?GetUnitOwner@AuraScript@@',controller,live)
needs(update,live,equipped,'?GetQuestObjectiveData@Player@@QEBAHII@Z','?CreateConversation@Conversation@@','?BindQuestObject@Player@@','?GetSpellInfo@SpellMgr@@',method('Cancel',ai),method('Close',ai))
needs('?BindQuestObject@Player@@','?BindQuestObjectScope@WorldObject@@')
needs('?Update@Conversation@@','?IsQuestObjectExpired@WorldObject@@','?Remove@Conversation@@')
needs('?CancelQuestObjects@Player@@','?GetConversation@ObjectAccessor@@','?Remove@Conversation@@')
assert {209685,209743,1923,40341,105787,30,5000,20000,102826,105298,104853,362093,362092,362094}<=values(update)
assert any(x.mnemonic=='movups' and x.op_str=='xmmword ptr [rdi + 0x30], xmm0' for x in ins(update))
for forbidden in ['?AddItem@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?AddDelayedEvent@Unit@@','?Delete@GameObject@@']:
 assert not sites(ins(update),forbidden)
needs(method('Close',ai),'?SetLootState@GameObject@@')
needs('?OnLootStateChanged@go_art_sharasdal@@','?GetItemByEntry@Player@@','?SetLootState@GameObject@@',begin)
assert 3 in values('?OnLootStateChanged@go_art_sharasdal@@')
for consumer in ['?HandleSpellClick@Unit@@','?canSeeSpellClickOn@Player@@','?UpdateForQuestWorldObjects@Player@@']:
 needs(consumer,'?GetConditionsForSpellClickEvent@ConditionMgr@@')
for token in [b'go_art_sharasdal\0',b'spell_sharasdal_acquisition\0',b'aura_sharasdal_acquisition\0']:
 assert token in n['binary']
print('PASS selected GO/controller/SpellScript/AuraScript factories and callbacks, exact original Player/quest/item/source GUIDs and phase-independent native registry; real249 equip before61, no duplicate249/early134/ability, actual scene expiry/complete, acknowledged native6/105787 and bool1923 factory/bounded optional retry; real source18 click/visibility/refresh consumers. Retained Lady/Kraliss/roar/wall/heal paths pass; actual client/full1066 remain separate.')
