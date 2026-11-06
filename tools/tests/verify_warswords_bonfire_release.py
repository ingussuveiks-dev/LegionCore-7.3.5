"""Verify the installed Fury bonfire AI and post-lock Opening dispatch."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n,ins=p['native'],p['instructions']
def sites(body,prefix):
 targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
 assert targets,prefix
 return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]
def vals(body):return {o.imm for x in body for o in x.operands if o.type==CS_OP_IMM}
opening=ins('?EffectOpenLock@Spell@@')
lock=sites(opening,'?CanOpenLock@Spell@@')[0]
start=next(i for i,x in enumerate(opening) if x.mnemonic=='cmp' and any(o.type==CS_OP_IMM and o.imm==198513 for o in x.operands))
loot=sites(opening,'?SendLoot@Spell@@')[-1]
assert lock<start<loot
block=opening[start:loot]
assert {1511,245188,134117,10,2474}<=vals(block),'Native bonfire scope lost'
assert sites(block,'?GetScriptName@ObjectMgr@@'), 'Binding guard absent'
hello=[i for i,x in enumerate(block) if x.mnemonic=='mov' and len(x.operands)==2 and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==0x48]
assert hello and block[hello[0]+1].op_str=='r8b, 1' and block[hello[0]+4].mnemonic=='call' and block[hello[0]+4].op_str=='r9','Completed-use dispatch missing'
rejected=block[1].operands[0].imm
for i,x in enumerate(block[:hello[0]]):
 if x.mnemonic=='cmp' and any(o.type==CS_OP_IMM and o.imm in {1511,245188,134117,10,2474} for o in x.operands):
  assert block[i+1].mnemonic=='jne' and block[i+1].operands[0].imm==rejected,'Tuple mismatch can dispatch ignition'
method='?GossipHello@go_mystic_bonfireAI@go_mystic_bonfire@@'
body=ins(method);credit=sites(body,'?UpdateAchievementCriteria@Player@@')
assert len(credit)==1 and {1511,245188,40043,944,47272,92}<=vals(body)
for dependency in ('?CanContact@Player@@','?GetScenario@ScenarioMgr@@','?GetCurrentStep@Scenario@@','?GetQuestRewardStatus@Player@@','?IsWithinLOSInMap@WorldObject@@'):
 assert sites(body,dependency)[0]<credit[0],dependency
phase=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xa0]
assert len(phase)==1 and phase[0]>credit[0] and body[phase[0]-1].op_str=='edx, 4' and body[phase[0]-2].op_str=='r8b, 1'
assert sites(body,'?GetGameObjectListWithEntryInGrid@WorldObject@@')[0]>credit[0]
phase_target=n['symbol']('?SetPhaseMask@GameObject@@')
vtable=[]
for x in ins('??0GameObject@@'):
 for o in x.operands:
  if x.mnemonic=='lea' and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
   a=x.address+x.size+o.mem.disp
   if struct.unpack_from('<Q',n['binary'],n['offset'](a)+0xa0)[0]==phase_target:vtable.append(a)
assert vtable,'Native GameObject constructor phase slot changed'
assert any(x.mnemonic=='mov' and x.op_str.endswith(', 1') and x.operands[0].type==CS_OP_MEM and x.operands[0].size==1 for x in body[:credit[0]]),'Reentrant ignition claim absent'
for forbidden in ('?AddItem@Player@@','?TeleportTo@Player@@','?CompleteQuest@Player@@','?AddDelayedEvent@Unit@@'):
 targets={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(forbidden)}
 assert not targets & n['calls'](method),forbidden
factory=ins('?GetAI@go_mystic_bonfire@@')
tables=[]
for x in factory:
 for o in x.operands:
  if x.mnemonic=='lea' and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
   off=n['offset'](x.address+x.size+o.mem.disp)
   if struct.unpack_from('<Q',n['binary'],off+0x48)[0]==n['symbol'](method):tables.append(off)
assert len(tables)==1,'Bonfire factory does not select repaired AI'
print('PASS selected Release bonfire AI: active Warrior/context/native47272 once-only claim and post-advance fog cleanup; exact198513/245188/134117/1511/goober/2474/binding dispatch follows real CanOpenLock and precedes SendLoot. No teleport/item/quest grants. Native traversal is checked separately.')
