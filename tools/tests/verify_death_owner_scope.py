from pathlib import Path
import json,subprocess
from five_ten_20261111_scope import retained_source
def verify(plan_path,base):
 r=Path(__file__).resolve().parents[2];plan=json.loads(Path(plan_path).read_text());assert len(plan)==10
 texts={p:retained_source(p)for e in plan for p in e['paths']}
 for e in reversed(plan):
  for ed in reversed(e['edits']):
   p=ed['path'];assert texts[p].count(ed['new'])==1,e['key'];texts[p]=texts[p].replace(ed['new'],ed['old'],1)
 for p,text in texts.items():assert text==subprocess.check_output(['git','show',base+':'+p]).decode('utf-8-sig').replace('\r\n','\n'),p
 print('PASS full-file reversal of ten death repairs:only explicit eligible-owner/victim-instance edits;original IDs,admission,cleanup and native group/instance APIs retained. No SQL/core/schema/packet/character changes')
