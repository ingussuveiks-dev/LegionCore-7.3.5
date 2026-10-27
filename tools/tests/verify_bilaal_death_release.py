"""Verify the registered1583 instance and real native death/receiver consumers."""
from pathlib import Path
import re
import runpy
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM,CS_OP_REG

p=runpy.run_path(str(Path(__file__).with_name('verify_lights_wrath_takeover_release.py')))
n,ins,values,references,table,needs=(p[k] for k in ('n','ins','values','references','table','needs'))
native_ins=ins
def ins(method):
 try:return native_ins(method)
 except AssertionError as error:
  if not str(error).startswith('Missing runtime bounds:'):raise
  address=n['symbol'](method)
  following=min(a for a in n['symbols'].values() if a>address)
  assert following-address<=64,'Unbounded non-leaf function'
  decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
  return list(decoder.disasm(n['binary'][n['offset'](address):n['offset'](address)+following-address],address))
def values(method):return {o.imm for x in ins(method) for o in x.operands if o.type==CS_OP_IMM}
factory='?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_priest_mage_nexus_art'
registration=references('?AddSC_instance_class_art_scenarios@@')
assert any(n['symbol'](factory) in table(a,12) for a in registration)
death='?CreatureDies@instance_priest_mage_nexus_art@@'
update='?Update@instance_priest_mage_nexus_art@@'
assert any(n['symbol'](death) in table(a,140) and n['symbol'](update) in table(a,140) for a in references(factory))
needs(death,'?Death@State@BilaalDeath@@')
needs(update,'?Try@State@BilaalDeath@@')
assert 2000 in values(update)
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
for name,data in (
 ('priest',(104502,370522,1065,41631,5,6140,47117)),
 ('mage',(106708,370551,1101,42011,8,6068,47995))):
 matches=re.findall(r'\?'+name+r'@BilaalDeath@@3URoute@1@B\s+([0-9a-fA-F]{16})\s',text)
 assert len(matches)==1
 address=int(matches[0],16)
 assert n['binary'][n['offset'](address):n['offset'](address)+28]==struct.pack('<7I',*data)
 assert address in references('?Match@BilaalDeath@@')
# Derive the actual native player-damage field from Unit::Kill's refusal path,
# rather than accepting a nearby unused constant or a test-host field layout.
kill=ins('?Kill@Unit@@')
clear=n['symbol']('?SetLootRecipient@Creature@@')
site=next(i for i,x in enumerate(kill) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==clear)
window=kill[max(0,site-30):site]
fields=[]
for i,x in enumerate(window):
 if x.mnemonic!='cmp' or len(x.operands)!=2 or x.operands[0].type!=CS_OP_MEM or x.operands[0].size!=4:continue
 rhs=x.operands[1]
 zero=rhs.type==CS_OP_IMM and rhs.imm==0
 if rhs.type==CS_OP_REG:
  # MSVC also emits cmp [creature+field],esi after xor esi,esi.
  writes=[q for q in window[:i] if q.operands and q.operands[0].type==CS_OP_REG and q.operands[0].reg==rhs.reg
          and q.mnemonic not in ('cmp','test')]
  zero=bool(writes and writes[-1].mnemonic=='xor' and len(writes[-1].operands)==2
            and writes[-1].operands[1].type==CS_OP_REG and writes[-1].operands[1].reg==rhs.reg)
 if zero and window[i+1].mnemonic=='ja':fields.append(x.operands[0].mem.disp)
assert len(fields)==1,fields
matched=ins('?Match@BilaalDeath@@')
guard=next(i for i,x in enumerate(matched) if x.mnemonic=='cmp' and len(x.operands)==2 and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==fields[0] and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==0)
assert matched[guard+1].mnemonic in ('je','jbe')
needs('?Death@State@BilaalDeath@@','?Match@BilaalDeath@@','?GetLootRecipient@Creature@@',
 '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?GetFirstMember@Group@@QEBAPEBVGroupReference@@',
 '?Eligible@BilaalDeath@@','?getQuestStatus@Player@@','?GetPhases@WorldObject@@','?Try@State@BilaalDeath@@')
needs('?Eligible@BilaalDeath@@','?InSamePhase@WorldObject@@QEBA_NPEBV1@@',
 '?IsAtGroupRewardDistance@Player@@','?GetQuestObjectiveData@Player@@QEBAHII',
 '?GetQuestRewardStatus@Player@@','?getQuestStatus@Player@@')
needs('?Original@State@BilaalDeath@@','?getQuestStatus@Player@@')
needs('?Try@State@BilaalDeath@@','?Original@State@BilaalDeath@@',
 '?UpdateAchievementCriteria@Player@@','?Victory@BilaalDeath@@','?CreateReceiver@State@BilaalDeath@@')
receipt='?CurrentReceipt@State@BilaalDeath@@'
if any(name.startswith(receipt) for name in n['symbols']):needs('?Try@State@BilaalDeath@@',receipt)
else:receipt='?Try@State@BilaalDeath@@' # MSVC inlines this small admission predicate.
needs(receipt,'?InSamePhaseId@WorldObject@@QEBA_NAEBV',
 '?GetQuestObjectiveData@Player@@QEBAHII','?GetQuestRewardStatus@Player@@')
needs('?Try@State@BilaalDeath@@','?GetUnit@ObjectAccessor@@')
assert 49810 in values('?Try@State@BilaalDeath@@')
spawn='?CreateReceiver@State@BilaalDeath@@'
needs(spawn,'??0TempSummon@@','?Create@Creature@@','?SetGuidValue@Object@@',
 '?SetHomePosition@Creature@@QEAAXAEBUPosition@@','??$AddToMap@VCreature@@@Map@@','?SetTempSummonType@TempSummon@@')
assert {104546,8} <= values(spawn)
body=ins(spawn)
add=n['symbol']('??$AddToMap@VCreature@@@Map@@')
site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==add)
tested=next(i for i in range(site+1,site+5) if body[i].mnemonic=='test' and body[i].op_str=='al, al')
assert not any(x.mnemonic=='call' for x in body[site+1:tested])
assert body[tested+1].mnemonic=='jne'
success=body[tested+1].operands[0].imm
# Devirtualize against the actual constructed TempSummon table. The phase and
# initialization hooks must belong to this factory's concrete receiver.
vtables=[(a,table(a,300)) for a in references('??0TempSummon@@')]
vtable=next(t for _,t in vtables if n['symbol']('?InitStats@TempSummon@@') in t and n['symbol']('?InitSummon@TempSummon@@') in t)
virtuals={key:vtable.index(n['symbol'](symbol))*8 for key,symbol in (
 ('stats','?InitStats@TempSummon@@'),('init','?InitSummon@TempSummon@@'),('phase','?SetPhaseId@Unit@@'),
 ('cleanup','?CleanupsBeforeDelete@Unit@@'))}
for key in ('stats','phase'):
 assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==virtuals[key] for x in body[:site]),key
for key in ('init','cleanup'):
 assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==virtuals[key] for x in body[site+3:]),key
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==virtuals['cleanup'] for x in body[tested+2:] if x.address<success)
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==virtuals['init'] for x in body if x.address>=success)
print('PASS registered1583 instance factory/vtable uses real AI-independent death and2s update; exact dual Route objects drive native kill-owner/group/quest-objective/lease consumers and accepted49810 before receiver creation. Actual native TempSummon constructor/Create, captured phase hook, stats, checked map insertion, rollback cleanup, InitSummon and manual8 dispatch verified. Native host owns failure/retake/source-removal/reentrancy coverage; downstream/client sequence remains separate.')
