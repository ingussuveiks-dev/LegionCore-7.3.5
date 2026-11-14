"""Exact natural-expiry completion bridge; no general scene policy change."""
from pathlib import Path
import functools,hashlib,json,subprocess
from artifact_later_command_scope import normalize as normalize_command_amendments,verify as verify_command_amendments
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/WarswordsSceneExpiryRepair.json').read_text())
def normalize(path,source):
 source=normalize_command_amendments(path,source)
 for row in reversed(PLAN['changes']):
  if row['path']!=path:continue
  if source.count(row['new'])==1:source=source.replace(row['new'],row['old'],1)
  else:assert row['old'] and source.count(row['old'])==1,('Partial finite-scene amendment',path)
 return source
@functools.lru_cache(None)
def verify():
 verify_command_amendments()
 for path,hashes in PLAN['files'].items():
  old=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode().replace('\r\n','\n')
  current=normalize_command_amendments(path,(ROOT/path).read_text(encoding='utf-8-sig'));assert hashlib.sha256(old.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(current.encode()).hexdigest()==hashes['after_sha256']
  for row in reversed(PLAN['changes']):
   if row['path']==path:
    assert current.count(row['new'])==1;current=current.replace(row['new'],row['old'],1)
  assert current==old,('Unrelated finite-scene change',path)
 return True
