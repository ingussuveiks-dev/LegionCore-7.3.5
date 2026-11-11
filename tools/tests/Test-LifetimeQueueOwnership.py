from pathlib import Path
import json
from lifetime_queue_ownership import queue_owner
r=Path(__file__).resolve().parents[2];units=owned=0
for name in ['FortySecond','FortyThird','FortyFourth','FortyFifth']:
 for e in json.loads((r/f'tools/tests/{name}TenRepairs.json').read_text()):
  source=(r/e['paths'][0]).read_text(encoding='utf-8-sig')
  for cb in e['callbacks']:assert queue_owner(e,source,cb)=='Unit';units+=1
for name in ['Fortieth','FortyFirst']:
 for e in json.loads((r/f'tools/tests/{name}TenRepairs.json').read_text()):
  if not e.get('withdrawn'):continue
  source=(r/e['paths'][0]).read_text(encoding='utf-8-sig')
  for cb in e['callbacks']:
   assert queue_owner(e,source,cb)=='AI';owned+=1
   assert queue_owner(e,source,cb|{'delegated':False})=='Unit'
   assert queue_owner(e,source,cb|{'queue':'combat'})=='Unit'
assert (units,owned)==(202,12)
try:queue_owner({'cls':'Unproven'},'struct Unproven : UnknownAI { };',{'queue_owner':'me','delegated':True,'queue':'event'})
except ValueError:pass
else:raise AssertionError('Unknown overload ownership must not be guessed')
try:queue_owner({'cls':'LocalQueue'},'struct LocalQueue : ScriptedAI {void AddDelayedEvent(unsigned,void*);};',{'queue_owner':'me','delegated':True,'queue':'event'})
except ValueError:pass
else:raise AssertionError('A local shadowing overload must be inspected')
print('PASS actual-source overload classification:202Unit registrations,12withdrawn AI-owned registrations;explicit Creature calls/combat overloads distinguished,unknown ancestry rejected. Actual native queue dispatch/destruction checked separately.')
