"""Verify real Light's Wrath physical admission and retained native consumers."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_contact_release.py')))
n,ins=p['n'],p['p']['instructions']
body=ins('?Use@ArtifactAcquisition@@')
def cmp(x,v):return x.mnemonic=='cmp' and len(x.operands)==2 and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==v
first=next(i for i,x in enumerate(body) if cmp(x,248572))
guard=body[first:]
reject={}
for x,y in zip(guard,guard[1:]):
 for v,branch in ((1583,'jne'),(1065,'jne'),(6,'jb'),(7,'ja')):
  if cmp(x,v):
   assert y.mnemonic==branch and y.operands[0].type==CS_OP_IMM
   reject[v]=y.operands[0].imm
assert reject.keys()=={1583,1065,6,7} and len(set(reject.values()))==1
assert guard[1].mnemonic=='jne' and guard[1].operands[0].imm>guard[1].address
failure=next(x for x in body if x.address==reject[6]);assert failure.mnemonic=='mov' and failure.op_str=='al, 1'
for dependency in ('?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@','?GetCurrentStep@Scenario@@'):
 assert n['symbol'](dependency) in n['calls']('?Use@ArtifactAcquisition@@')
assert n['symbol']('?GetConditionsForNotGroupedEntry@ConditionMgr@@') in n['calls']('?CheckCast@Spell@@')
assert n['symbol']('?IsObjectMeetingSmartEventConditions@ConditionMgr@@') in n['calls']('?ProcessEventsFor@SmartScript@@')
assert n['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEAA_NPEAVWorldObject@@AEBV') in n['calls']('?AllowedForPlayer@LootItem@@')
assert n['symbol']('?LootItemInSlot@Loot@@') in n['calls']('?StoreLootItem@Player@@')
print('PASS actual selected acquisition guard scopes248572 to1583/1065/exact6..7 before native loot/owned replay; retained native source4 slot, SmartEvent and cast condition consumers. SQL/native host own cached/stale/deferred predicates; actual client pending.')
