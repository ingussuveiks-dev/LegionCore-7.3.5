from pathlib import Path
import json,subprocess,re
r=Path(__file__).resolve().parents[2];plan=json.loads((r/'tools/tests/TwentiethTenRepairs.json').read_text());assert len(plan)==len({e['key']for e in plan})==10
texts={p:(r/p).read_text(encoding='utf-8-sig')for e in plan for p in e['paths']}
for e in reversed(plan):
 for edit in reversed(e['edits']):assert texts[edit['path']].count(edit['new'])==edit['count'];texts[edit['path']]=texts[edit['path']].replace(edit['new'],edit['old'])
for p,s in texts.items():assert re.sub(r'\s','',s)==re.sub(r'\s','',subprocess.check_output(['git','show','f38d9173743174d40727c39801e1ef91e9a286b0:'+p]).decode('utf8')),p
print('PASS exactly10 focused repairs;reversing selected edits reproduces pinned base;unrelated behavior unchanged')
