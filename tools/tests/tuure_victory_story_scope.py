"""Exact optional2009 amendment; retain the certified2007/progression sources."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/TuureVictoryStoryRepair.json').read_text())
@functools.lru_cache(None)
def verify():
 restored={}
 sha=lambda s:hashlib.sha256(s.encode()).hexdigest()
 for f in PLAN['files']:
  source=(ROOT/f['path']).read_text(encoding='utf-8-sig')
  old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+f['path']]).decode('utf-8-sig').replace('\r\n','\n')
  assert sha(source)==f['after_sha256'] and sha(old)==f['before_sha256']
  for edit in reversed(f['edits']):
   assert source.count(edit['new'])==1,'Partial or foreign first-victory-dialogue amendment'
   source=source.replace(edit['new'],edit['old'],1)
  assert source==old,'Other route/middle/credit/ownership source changed'
  restored[f['path']]=source
 assert sha((ROOT/PLAN['header']).read_text())==PLAN['header_sha256']
 return restored
if __name__=='__main__':
 verify();print('PASS exact optional2009 amendment;retained2007/2039/route/progression sources unchanged')
