"""Verify direct actor captures in all twenty-six linked closures."""
from pathlib import Path
import json,runpy,re
from verify_actor_queue_capture import check_actor_capture
p=runpy.run_path(str(Path(__file__).with_name('verify_twentyeighth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('TwentyNinthTenRepairs.json').read_text(encoding='utf8'));seen=[]
for e in plan:
 found=[]
 for method in sorted({c['method']for c in e['callbacks']}):
  for cb in callbacks(e['cls'],method):
   r=expanded(cb)
   # The separately repaired cross-pull closure shares EnterCombat; select only this dialogue closure.
   if e['key']=='InquisitionDialogue' and method=='EnterCombat' and not sites(r,'?Talk@CreatureAI@@'):continue
   assert any(x.mnemonic=='mov'and re.fullmatch(r'r\w+, qword ptr \[rcx \+ 8\]',x.op_str)for x in r),(e['key'],'captured actor load')

   try:check_actor_capture(p,r)
   except AssertionError as error:raise AssertionError((e['key'],method,str(error)))
   found.append(cb)
 assert len(found)==sum(c.get('compiled_count',1)for c in e['callbacks']),(e['key'],len(found));seen+=found
assert len(seen)==26
print('PASS actual Release twentyninth ten:twenty-six actor-owned closures directly load their captured Creature without dereferencing retired AI;all ten live-bound controllers and original timing/payload source scopes retained. Prior287 selected checks retained,total297;ASAN/native/client scopes remain separate.')

extra=json.loads(Path(__file__).with_name('HasabelWaveFollowup.json').read_text(encoding='utf8'))[0]
cb,=callbacks(extra['cls'],extra['callbacks'][0]['method']);check_actor_capture(p,expanded(cb))
print('PASS uncounted Hasabel Felcrush follow-up closure;original delay argument and captured Position retained')
