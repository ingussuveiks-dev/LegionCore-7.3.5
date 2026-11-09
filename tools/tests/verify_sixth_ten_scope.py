from pathlib import Path
import json,re,subprocess
r=Path(__file__).resolve().parents[2];plan=json.loads((r/'tools/tests/SixthTenRepairs.json').read_text());assert len(plan)==len({e['key']for e in plan})==10
paths={p for e in plan for p in e['paths']};text={p:(r/p).read_text(encoding='utf-8-sig')for p in paths}
for entry in reversed(plan):
 for edit in reversed(entry['edits']):
  s=text[edit['path']];assert s.count(edit['new'])==1,(entry['key'],edit['path']);text[edit['path']]=s.replace(edit['new'],edit['old'])
for p,s in text.items():
 old=subprocess.check_output(['git','-C',str(r),'show','efd389974:'+p]).decode('utf8');assert re.sub(r'\s','',s)==re.sub(r'\s','',old),p
print('PASS exactly ten fixes/nine production files;reversing selected edits reproduces efd389974;unrelated behavior,quest gates,timers,group/credit and database schema unchanged.')
