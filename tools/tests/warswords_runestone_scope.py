"""Exact scoped native rune completion/instance receipt repair;no client certificate."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/WarswordsRunestoneRepair.json').read_text(encoding='utf-8'))
def normalize(path,source):
 for row in reversed(PLAN['changes']):
  if row['path']!=path:continue
  if source.count(row['new'])==1:source=source.replace(row['new'],row['old'],1)
  else:assert row['old']=='' or source.count(row['old'])==1,('Partial runestone repair',path)
 return source
@functools.lru_cache(None)
def verify():
 for path,hashes in PLAN['files'].items():
  pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  current=(ROOT/path).read_text(encoding='utf-8-sig')
  assert hashlib.sha256(pinned.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(current.encode()).hexdigest()==hashes['after_sha256']
  assert normalize(path,current)==pinned,('Unrelated rune change',path)
 return True
if __name__=='__main__':verify();print('PASS exact three-file runestone completion source scope;deployment/client checks separate')
