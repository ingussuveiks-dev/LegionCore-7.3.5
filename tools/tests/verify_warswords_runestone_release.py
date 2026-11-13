"""Selected linked rune callback/admission/ACK/native-presentation proof."""
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
import verify_warswords_bonfire_interaction_release as retained
from warswords_runestone_scope import verify
from five_ten_20261111_pe import ins as bounded_instructions
verify();n,ins,sites=retained.n,retained.ins,retained.sites
pairs,reaches=retained.pairs,retained.reaches
scope='@go_prison_runestoneAI@go_prison_runestone@@'
use=ins('?GossipUse'+scope);hello=ins('?GossipHello'+scope);eligible=ins('?Eligible'+scope);counter=ins('?Counter'+scope)
opening=ins('?EffectOpenLock@Spell@@');lock,=sites(opening,'?CanOpenLock@Spell@@')
start,=[i for i,x in enumerate(opening)if i>lock and x.mnemonic=='cmp'and any(o.type==CS_OP_IMM and o.imm==8386 for o in x.operands)]
slot=next(i for i in range(start,len(opening))if opening[i].mnemonic=='mov'and any(o.type==CS_OP_MEM and o.mem.disp==0x48 for o in opening[i].operands))
block=opening[start:slot+5];reject=block[1].operands[0].imm
assert slot<sites(opening,'?SendLoot@Spell@@')[-1]
for value,jump in ((8386,'jne'),(1511,'jne'),(244940,'jne'),(134130,'jb'),(134133,'ja'),(10,'jne'),(99,'jne')):
 i,=[i for i,x in enumerate(block)if x.mnemonic=='cmp'and any(o.type==CS_OP_IMM and o.imm==value for o in x.operands)]
 assert block[i+1].mnemonic==jump and block[i+1].operands[0].imm==reject,('Unrelated tuple reaches rune callback',value)
assert sites(block,'?GetScriptName@ObjectMgr@@')
assert block[-4].op_str=='r8b, 1'and block[-1].mnemonic=='call'and block[-1].op_str=='r9'
assert any(x.mnemonic=='lea'and any(o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP and n['binary'][n['offset'](x.address+x.size+o.mem.disp):n['offset'](x.address+x.size+o.mem.disp)+19]==b'go_prison_runestone'for o in x.operands)for x in block),'Exact native ScriptName missing'
admit,=sites(use,'?Eligible'+scope);busy,=sites(use,'?IsNonMeleeSpellCast@Unit@@');cast,=sites(use,'?CastSpell@Unit@@')
assert admit<busy<cast and ('xor','r8d, r8d')in pairs(use[admit-4:admit])
assert {('mov','r8d, 0x20c2'),('xor','r9d, r9d')}<=pairs(use[cast-12:cast]),'Use must request native8386,nontriggered'
for point,branch in ((admit,'je'),(busy,'jne')):
 assert use[point+1].mnemonic=='test'and use[point+2].mnemonic==branch
 assert use[cast].address not in reaches(use,use[point+2].operands[0].imm)
assert {('mov','rax, qword ptr [rbx + 0x18]'),('test','rax, rax'),('cmp','rax, rdi')}<=pairs(use),'Only the actual authorized presentation owner may pass generic Use'
credit,=sites(hello,'?UpdateAchievementCriteria@Player@@');present,=sites(hello,'?Use@GameObject@@');check,=sites(hello,'?Eligible'+scope)
assert check<credit<present and ('mov','r8b, 1')in pairs(hello[check-4:check])
assert {('mov','edx, 0x5c'),('mov','r8d, 0xb77a'),('mov','byte ptr [rsi + 0x10], 1')}<=pairs(hello[:credit]),'Claim before native46970 event'
assert any(x.mnemonic=='cmp'and x.op_str=='rax, 4'for x in hello[:credit])
assert len(sites(hello,'?Counter'+scope))==2 and len(sites(hello,'?GetCurrentStep@Scenario@@'))==2
from collections import Counter
failure_targets=Counter(x.operands[0].imm for x in hello[credit:present]if x.mnemonic=='jne'and x.operands[0].type==CS_OP_IMM)
fail,=[target for target,count in failure_targets.items()if count==3]
assert hello[present].address not in reaches(hello,fail),'Failed native ACK can present destruction'
assert sum(x.mnemonic=='jne'and x.operands[0].type==CS_OP_IMM and x.operands[0].imm==fail for x in hello[credit:present])==3,'Scenario identity/last-count/native next-step ACK gates missing'
assert ('mov','qword ptr [rsi + 0x18], rbp')in pairs(hello[:present])and ('mov','qword ptr [rsi + 0x18], r12')in pairs(hello[present:])and ('xor','r12d, r12d')in pairs(hello[:credit]),'Synchronous native presentation must own and clear its exact Player'
assert any(x.mnemonic=='mov'and any(o.type==CS_OP_MEM and o.mem.disp==0xb0 for o in x.operands)for x in hello[:credit]),'Instance receipt readback must precede credit'
assert any(x.mnemonic=='mov'and any(o.type==CS_OP_MEM and o.mem.disp==0xb8 for o in x.operands)for x in hello[:credit]),'Instance receipt must be claimed before credit'
assert any(x.mnemonic=='call'and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xb8 and x.address>=fail for x in hello),'Failed native ACK must roll back its instance receipt'
assert any(x.mnemonic=='cmp'and any(o.type==CS_OP_IMM and o.imm==45863 for o in x.operands)for x in counter)
for dependency in ('?CanContact@Player@@','?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@','?GetCurrentStep@Scenario@@','?IsWithinDistInMap@WorldObject@@'):
 assert sites(eligible,dependency),dependency
for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?TeleportTo@Player@@','?SetCurrentStep@Scenario@@'):
 assert not sites(hello,forbidden)and not sites(use,forbidden),forbidden
factory=ins('?GetAI@go_prison_runestone@@');tables=[]
for x in factory:
 for o in x.operands:
  if x.mnemonic=='lea'and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
   at=n['offset'](x.address+x.size+o.mem.disp)
   if struct.unpack_from('<Q',n['binary'],at+0x48)[0]==n['symbol']('?GossipHello'+scope):tables.append(at)
at,=tables;assert struct.unpack_from('<Q',n['binary'],at+0x40)[0]==n['symbol']('?GossipUse'+scope)
instance='@instance_warswords_InstanceMapScript@instance_warswords@@'
# This receipt getter is a genuine stack-free leaf with no unwind
# entry. Reuse the existing bounded native reader;do not enlarge its64-byte cap.
getter=bounded_instructions('?GetData'+instance);setter=ins('?SetData'+instance)
field,=[o.mem.disp for x in getter if x.mnemonic=='movzx'for o in x.operands if o.type==CS_OP_MEM]
assert {('add','edx, 0xfffdf40e'),('cmp','edx, 3')}<=pairs(getter),'Receipt getter must bound actual134130..134133'
assert any(x.mnemonic=='mov'and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==field for x in setter),'Receipt setter must write the same instance-owned array'
for value in (944,3,1):assert any(x.mnemonic=='cmp'and any(o.type==CS_OP_IMM and o.imm==value for o in x.operands)for x in setter)
for method in ('Initialize','onScenarionNextStep'):
 rows=ins('?'+method+instance)
 assert any(x.mnemonic=='mov'and x.operands[0].type==CS_OP_MEM and x.operands[0].size==4 and x.operands[0].mem.disp==field for x in rows),'Four native attempt receipts must be reset together: '+method
restored=ins('?OnGameObjectCreate'+instance)
assert any(x.mnemonic=='cmp'and any(o.type==CS_OP_MEM and o.mem.disp==field-134130 for o in x.operands)for x in restored),'Actual GO re-entry must consult its instance receipt'
assert sites(restored,'?SetGoState@GameObject@@'),'Destroyed presentation must be restored without credit'
factory=ins('?GetInstanceScript@instance_warswords@@');tables=[]
for x in factory:
 for o in x.operands:
  if x.mnemonic=='lea'and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
   at=n['offset'](x.address+x.size+o.mem.disp)
   if struct.unpack_from('<Q',n['binary'],at+0xb0)[0]==n['symbol']('?GetData'+instance):tables.append(at)
at,=tables;assert struct.unpack_from('<Q',n['binary'],at+0xb8)[0]==n['symbol']('?SetData'+instance)
assert n['symbol']('?OnGameObjectCreate'+instance)in struct.unpack_from('<80Q',n['binary'],at),'Actual instance factory must install restored GO presentation'
print('PASS linked real rune Use/Hello and instance factories,exact8386/99/four-spawn post-lock dispatch,normal cast/busy/eligibility,claimed46970/native45863 ACK,instance receipts/reload presentation/new-attempt reset and owner-scoped native destruction;retained full PE chain passes. Actual client and subsequent scene remain separate.')
