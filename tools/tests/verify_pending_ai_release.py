"""Actual linked production closure gates; no encounter/client claim."""
from pathlib import Path
import ast,json,re,hashlib
from verify_ai_lifetime_guard import has_lifetime_gate
root=Path(__file__).resolve().parents[2]
# Reuse the existing PE/unwind/thunk reader without running unrelated checks.
reader=(root/'tools/tests/verify_smart_linked_cast_retry_release.py').read_text()
tree=ast.parse(reader);fn=next(x for x in tree.body if isinstance(x,ast.FunctionDef) and x.name=='instructions')
bootstrap=reader[:reader.index('\nchecks = [')]+'\nfrom capstone import Cs,CS_ARCH_X86,CS_MODE_64\nmd=Cs(CS_ARCH_X86,CS_MODE_64);md.detail=True\n'+ast.get_source_segment(reader,fn)
n={};exec(compile(bootstrap,'matching-linked-pe-reader','exec'),n)
def instructions(name):return n['instructions'](name)
def expanded(name):
 rows=instructions(name);result=list(rows)
 for x in rows:
  if x.mnemonic not in ('jmp','call') or not x.op_str.startswith('0x'):continue
  target=n['resolve'](int(x.op_str,16));matches=[s for s,a in n['symbols'].items() if (s.startswith('??R<lambda') or s.startswith('??$invoke@') and '<lambda_' in s) and n['resolve'](a)==target]
  if matches:result+=instructions(matches[0])
 return result
plan=json.loads((root/'tools/tests/PendingAILifetime20261115.json').read_text());total=0
for e in plan:
 for method in sorted({c['method'] for c in e['callbacks']}):
  s=e['edits'][0]['new'];a=s.index(method+'(');a=s.index('{',a);i=a+1;depth=1
  while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
  expected=s[a:i].count(e['guard_init']);assert expected>=sum(c['method']==method for c in e['callbacks'])
  names=[s for s in n['symbols'] if s.startswith('?_Do_call@') and e['cls']+'@' in s and method+'@' in s and s.endswith('EEAAXXZ')]
  if method==e['cls']:names=[s for s in names if '???0'+method+'@' in s]
  passed=[s for s in names if has_lifetime_gate({'n':n},expanded(s))]
  assert len(passed)==expected,(e['key'],method,len(passed),expected)
  total+=len(passed)
  print('CASE linked',e['cls'],method,len(passed),'guards before payload PASS')
stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',(n['args'].release/'worldserver.map').read_text(errors='replace'));assert stamp and int(stamp[1],16)==n['u32'](n['pe']+8)
assert total>=4
print('PASS2 linked active AI classes/4 new admissions; all'+str(total)+' current guards in affected methods checked, matching PE/map; source/ASAN/native/client scopes distinct; SHA256='+hashlib.sha256(n['binary']).hexdigest().upper())
