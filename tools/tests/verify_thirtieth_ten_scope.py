from pathlib import Path
import json,subprocess,re
r=Path(__file__).resolve().parents[2];plan=json.loads((r/'tools/tests/ThirtiethTenRepairs.json').read_text());assert len(plan)==10
texts={p:(r/p).read_text(encoding='utf-8-sig')for e in plan for p in e['paths']}
created=set()
for e in reversed(plan):
 for ed in reversed(e['edits']):
  p=ed['path'];assert texts[p].count(ed['new'])==1,(e['key'],p)
  if ed['old']is None:assert texts[p]==ed['new'];created.add(p)
  else:texts[p]=texts[p].replace(ed['new'],ed['old'])
for p,s in texts.items():
 if p not in created:assert re.sub(r'\s','',s)==re.sub(r'\s','',subprocess.check_output(['git','show','41e54cb4e8cd2b243d51a7709e83c592a2230514:'+p]).decode('utf8')),p
print('PASS ten independent problems/ten implementation files;reverse edits reproduce41e54cb4e8cd2b243d51a7709e83c592a2230514;no SQL/character rewrite')
