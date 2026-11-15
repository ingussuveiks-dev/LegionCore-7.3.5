"""Reverse only the new admissions, preserving all earlier guards/payloads."""
from pathlib import Path
import json,subprocess,re,functools
from lifetime_queue_ownership import queue_owner
root=Path(__file__).resolve().parents[2]
plan=json.loads((root/'tools/tests/PendingAILifetime20261115.json').read_text())
def normalize(path,source):
 for e in plan:
  if e['paths'][0]!=path:continue
  ed=e['edits'][0]
  if source.count(ed['new'])==1:source=source.replace(ed['new'],ed['old'],1)
  else:assert source.count(ed['old'])==1,('Partial pending AI admission',path)
 return source
@functools.lru_cache(None)
def verify():
 assert len(plan)==2 and sum(len(e['callbacks']) for e in plan)==4
 for e in plan:
  path=e['paths'][0];source=(root/path).read_text(encoding='utf-8-sig');ed=e['edits'][0];assert source.count(ed['new'])==1;restored=ed['new']
  for cb in e['callbacks']:
   assert restored.count(cb['new'])==1
   capture='['+cb['capture']+', '+e['guard_init']+']';at=cb['new'].index('{',cb['new'].index(capture)+len(capture))+1
   assert cb['new'][at:].lstrip().startswith(e['guard'])
   body=cb['new'].replace(', '+e['guard_init'],'',1)
   body=re.sub(r'\n[ \t]*'+re.escape(e['guard']),'',body,count=1)
   assert body==cb['old'],(e['key'],'payload or delay changed')
   assert queue_owner(e,source,cb)=='Unit'
   restored=restored.replace(cb['new'],cb['old'],1)
  if e['added_member']:restored=restored.replace(e['edits'][0]['anchor_addition'],'',1)
  assert restored==ed['old'],(path,'class payload drift')
  old=subprocess.check_output(['git','-C',str(root),'show','cf1182f05:'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert source.replace(ed['new'],restored,1)==old,(path,'non-admission source drift')
  assert ed['new'].count(e['member'])==1
 return True
if __name__=='__main__':
 verify();print('PASS2 active AI classes/4 new admissions; exact reversal to cf1182f05, original payload/actors/timers/credit/cleanup and previous guards retained; Unit queue ownership proven')
