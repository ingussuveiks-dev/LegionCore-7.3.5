"""Verify direct actor captures in all twenty-six linked closures."""
from pathlib import Path
import json,runpy,re
from verify_actor_queue_capture import check_actor_capture
p=runpy.run_path(str(Path(__file__).with_name('verify_thirtyfirst_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('ThirtySecondTenRepairs.json').read_text(encoding='utf8'));seen=[]
for e in plan:
 found=[]
 for cls,method in sorted({(c['cls'],c['method'])for c in e['callbacks']}):
  for cb in callbacks(cls,method):
   # Constructor names otherwise match every member of the class, including AI-owned scheduler predicates.
   if method==cls and ('???0'+cls+'@@'not in cb or '@X$$V@std@@EEAAXXZ'not in cb):continue
   r=expanded(cb)
   try:check_actor_capture(p,r)
   except AssertionError as error:raise AssertionError((e['key'],method,str(error)))
   found.append(cb)
 assert len(found)==sum(c.get('compiled_count',1)for c in e['callbacks'])+e.get('retained_compiled_closures',0),(e['key'],len(found));seen+=found
assert len(seen)==31
print('PASS actual Release thirtysecond ten:twenty-six repaired plus five retained actor-owned closures directly load their captured Creature without dereferencing retired AI;all ten live-bound controllers and original timing/payload source scopes retained. Prior317 selected checks retained,total327;ASAN/native/client scopes remain separate.')
