from pathlib import Path
import json,subprocess,re
r=Path(__file__).resolve().parents[2];plan=json.loads((r/'tools/tests/ThirtyFourthTenRepairs.json').read_text());assert len(plan)==10
# The only body change is a first-statement weak-lifetime admission.
for e in plan:
 for ed in e['edits']:
  restored=ed['new'].replace(ed['anchor_addition'],'',1).replace(', '+e['guard_init'],'')
  restored=re.sub(r'(?:\n[ \t]*| )'+re.escape(e['guard']),'',restored)
  assert restored==ed['old']or(ed.get('trimmed_guard_trailing_whitespace')and [line.rstrip()for line in restored.splitlines()]==[line.rstrip()for line in ed['old'].splitlines()]),(e['key'],'payload/gates/timing changed')
  count=sum(c['cls']in re.findall(r'(?:struct|class)\s+(\w+)\s*:',ed['old'])for c in e['callbacks'])
  assert ed['new'].count(e['member'])==1
 for cb in e['callbacks']:
  capture='['+cb['capture']+', '+e['guard_init']+']';at=cb['new'].index('{',cb['new'].index(capture)+len(capture))+1
  assert cb['new'][at:].lstrip().startswith(e['guard']),(e['key'],'guard is not first statement')
# The only body change is a first-statement weak-lifetime admission.
for e in plan:
 for ed in e['edits']:
  restored=ed['new'].replace(ed['anchor_addition'],'',1).replace(', '+e['guard_init'],'')
  restored=re.sub(r'(?:\n[ \t]*| )'+re.escape(e['guard']),'',restored)
  assert restored==ed['old']or(ed.get('trimmed_guard_trailing_whitespace')and [line.rstrip()for line in restored.splitlines()]==[line.rstrip()for line in ed['old'].splitlines()]),(e['key'],'payload/gates/timing changed')
  count=sum(c['cls']in re.findall(r'(?:struct|class)\s+(\w+)\s*:',ed['old'])for c in e['callbacks'])
  assert ed['new'].count(e['member'])==1
 for cb in e['callbacks']:
  capture='['+cb['capture']+', '+e['guard_init']+']';at=cb['new'].index('{',cb['new'].index(capture)+len(capture))+1
  assert cb['new'][at:].lstrip().startswith(e['guard']),(e['key'],'guard is not first statement')
texts={p:(r/p).read_text(encoding='utf-8-sig')for e in plan for p in e['paths']}
created=set()
for e in reversed(plan):
 for ed in reversed(e['edits']):
  p=ed['path'];assert texts[p].count(ed['new'])==1,(e['key'],p)
  if ed['old']is None:assert texts[p]==ed['new'];created.add(p)
  else:texts[p]=texts[p].replace(ed['new'],ed['old'])
for p,s in texts.items():
 if p not in created:assert re.sub(r'\s','',s)==re.sub(r'\s','',subprocess.check_output(['git','show','8c7da6ef842a755dba2905fc9ed7696b1fa5d47f:'+p]).decode('utf8')),p
print('PASS ten independent problems/10 implementation files;reverse edits reproduce8c7da6ef842a755dba2905fc9ed7696b1fa5d47f;no SQL/character rewrite')
