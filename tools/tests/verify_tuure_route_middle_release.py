"""Selected linked Tuure middle dialogue and retained native route/kill paths.

Native behavioral tests are separate; this detects missing/stale link inputs.
"""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_first_route_release.py')))
n,ins,values=p['n'],p['instructions'],p['values']
scope='instance_tuure_InstanceMapScript@instance_tuure@@'
def method(name):return '?'+name+'@'+scope
complete=method('CompleteRouteMiddle')
def needs(name,*targets):
 for target in targets:
  addresses={n['resolve'](a) for s,a in n['symbols'].items() if s.startswith(target)}
  assert addresses and addresses&n['calls'](name),(name,target)
needs(complete,'?CreateConversation@Conversation@@','?BindQuestObject@Player@@','?Eligible@TuureFirstFight@@')
assert {2007,41993,1000,30,106137,361432}<=values(complete)
for target in ('?UpdateAchievementCriteria@Player@@','?UpdateAchievementCriteria@Scenario@@','?CompleteQuest@Player@@','?MovePath@MotionMaster@@QEAAXI_N'):
 assert not {n['resolve'](a) for s,a in n['symbols'].items() if s.startswith(target)}&n['calls'](complete),target
needs(method('UpdateRoute'),complete)
find='?FindCreatureActor@RouteMiddleConversation@'+scope
needs(find,'?GetCreature@ObjectAccessor@@')
assert n['symbol']('?FindNearestCreature@WorldObject@@') not in n['calls'](find)
refs=[]
for name in (complete,'??0RouteMiddleConversation@'+scope):
 if not any(s.startswith(name) for s in n['symbols']):continue
 for x in ins(name):
  for op in x.operands:
   if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:refs.append(x.address+x.size+op.mem.disp)
tables=[]
for address in refs:
 try:rows=struct.unpack_from('<120Q',n['binary'],n['offset'](address))
 except (ValueError,struct.error):continue
 if n['symbol'](find) in rows:tables.append(address)
assert len(set(tables))==1,'Actual CreateConversation request does not select captured-role conversation'
assert len(p['p']['tables'])==1 and n['symbol'](method('OnPlayerEnter')) in p['p']['tables'][0]
print('PASS selected Tuure checked native2007 creation/captured speakers/quest scope and unchanged kill/arrival dispatch;no client/audio claim')
