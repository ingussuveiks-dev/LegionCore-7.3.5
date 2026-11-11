"""Exact independent actor-class changes; original callbacks remain byte-pinned."""
from pathlib import Path
import json,re,subprocess
from lifetime_source_amendments import lifetime_baseline_view
def verify(plan_path,base):
 r=Path(__file__).resolve().parents[2];plan=json.loads(Path(plan_path).read_text(encoding='utf8'));assert len(plan)==10
 for e in plan:
  for ed in e['edits']:
   restored=ed['new'].replace(ed['anchor_addition'],'',1).replace(', '+e['guard_init'],'')
   restored=re.sub(r'(?:\n[ \t]*| )'+re.escape(e['guard']),'',restored)
   assert restored==ed['old'],(e['key'],'payload/gates/delays changed')
   assert ed['new'].count(e['member'])==1
  for cb in e['callbacks']:
   capture='['+cb['capture']+', '+e['guard_init']+']';at=cb['new'].index('{',cb['new'].index(capture)+len(capture))+1
   assert cb['new'][at:].lstrip().startswith(e['guard']),(e['key'],'first statement')
 texts={p:lifetime_baseline_view(p,(r/p).read_text(encoding='utf-8-sig'))for e in plan for p in e['paths']}
 for e in reversed(plan):
  for ed in reversed(e['edits']):
   p=ed['path'];assert texts[p].count(ed['new'])==1,(e['key'],p);texts[p]=texts[p].replace(ed['new'],ed['old'],1)
 for p,s in texts.items():assert s==subprocess.check_output(['git','-C',str(r),'show',base+':'+p]).decode('utf-8-sig').replace('\r\n','\n'),p
 print('PASS exact lifetime scope:10independent actor classes;complete file reversal to '+base+';original payloads,prerequisites,delays,credit,reset and cleanup retained. No SQL/core/packet/character change.')
