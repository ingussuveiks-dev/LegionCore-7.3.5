"""Actual linked named energy callbacks and roster retry path; client traversal separate."""
from pathlib import Path
import json,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_tenth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
plan=json.loads(Path(__file__).with_name('EleventhTenRepairs.json').read_text());seen=set();branches=0
for e in plan:
 if e['kind']!='power':continue
 for c in e['contexts']:
  prefix='?'+c['method']+'@'+c['cls']+'@'
  if prefix in seen:continue
  seen.add(prefix);rows=ins(prefix);calls=sites(rows,'?SetPowerType@Unit@@');expected=3 if e['key']=='Allies'else 1;assert len(calls)==expected,(e['key'],calls)
  for at in calls:
   assert op(rows[max(0,at-6):at],'mov','edx, 3'),(e['key'],'actual energy type argument')
   following=[i for i in sites(rows,'?SetMaxPower@Unit@@')if i>at];assert following and following[0]-at<22,(e['key'],'type precedes maximum setup')
  if e['key']in ('Ysondre','Dragons'):
   zero,=sites(rows,'?SetPower@Unit@@');maximum,=sites(rows,'?SetMaxPower@Unit@@');assert maximum<zero and op(rows[max(0,zero-6):zero],'xor','r8d, r8d'),'explicit zero reset before retained empty-energy spell'
  assert 100 in values(rows)
  if e['key']=='Cenarius':assert op(rows,'mov','r8d, 0x5a')
  if e['key']=='Phemos':assert op(rows,'mov','r8d, 0x62')
  if e['key']=='Allies':assert {80,100,56}<=values(rows)
  branches+=len(calls)
assert branches==13 and len(seen)==11
rows=ins('?UpdateAI@npc_announcer_toc5AI@');lookups=sites(rows,'?GetPlayer@Unit@@');assert len(lookups)==2
first,second=lookups
assert rows[first+1].mnemonic=='test' and rows[first+2].mnemonic=='jne'
assert any(x.mnemonic=='jne'and x.operands[0].imm<=rows[first].address for x in rows[first+3:second]),'missing GUID pruning loop retries actual roster'
assert any(x.mnemonic=='call'for x in rows[first+3:second]),'erase missing roster node'
announces=sites(rows,'?AnnounceChampion@npc_announcer_toc5AI@');assert any(second<i<second+10 for i in announces),'actual resolved cached player announced'
assert 8000 in values(rows),'original intro/next-event delay retained'
assert op(rows[:first],'cmp','qword ptr [rsi + 0x170], r15'),'cached roster empty guard retained'
print('PASS actual Release eleventh ten:13resource initialization branches/11named callbacks for9controller fixes(type3 before100max;Cenarius90/Phemos98/allies80-100-56),four explicit dragon zero-resets independent of cast success,and actual roster player lookup/prune/retry/one announcement/original8s schedule. Prior107 selected paths retained;source/native/client scopes distinct.')
