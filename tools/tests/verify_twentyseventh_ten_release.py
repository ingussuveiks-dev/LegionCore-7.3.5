"""Verify direct actor captures in all eleven linked closures."""
from pathlib import Path
import json,runpy,re
p=runpy.run_path(str(Path(__file__).with_name('verify_twentysixth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('TwentySeventhTenRepairs.json').read_text(encoding='utf8'));seen=[]
for e in plan:
 found=[]
 for method in sorted({c['method']for c in e['callbacks']}):
  for cb in callbacks(e['cls'],method):
   r=expanded(cb);assert any(x.mnemonic=='mov'and re.fullmatch(r'r\w+, qword ptr \[rcx \+ 8\]',x.op_str)for x in r),(e['key'],'captured actor load')
   assert not any(re.search(r'qword ptr \[(?:rax|rbx|rcx|rdx|rdi|rsi) \+ 0x20\]',x.op_str)for x in r),(e['key'],'retired AI me dereference')
   found.append(cb)
 assert len(found)==len(e['callbacks']),(e['key'],len(found));seen+=found
assert len(seen)==11
print('PASS actual Release twentyseventh ten:eleven actor-owned closures directly load their captured Creature without dereferencing retired AI;all ten live-bound controllers and original timing/payload source scopes retained. Prior267 selected checks retained,total277;ASAN/native/client scopes remain separate.')
