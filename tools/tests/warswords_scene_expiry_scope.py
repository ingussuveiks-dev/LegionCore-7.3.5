"""Exact natural-expiry completion bridge; no general scene policy change."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/WarswordsSceneExpiryRepair.json').read_text())
@functools.lru_cache(None)
def verify():
 for path,hashes in PLAN['files'].items():
  old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode().replace('\r\n','\n')
  current=(ROOT/path).read_text();assert hashlib.sha256(old.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(current.encode()).hexdigest()==hashes['after_sha256']
  for row in reversed(PLAN['changes']):
   if row['path']==path:
    assert current.count(row['new'])==1;current=current.replace(row['new'],row['old'],1)
  assert current==old,('Unrelated finite-scene change',path)
 return True
