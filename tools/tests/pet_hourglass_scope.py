"""Exact native Forbidden Hourglass amendment, independently pinned before/after."""
from pathlib import Path
import functools,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
PLAN=json.loads((ROOT/'tools/tests/PetBattleHourglassAmendments.json').read_text(encoding='utf8'))
def normalize(path,source):
 for edit in reversed(PLAN['edits']):
  if edit['path']!=path:continue
  if source.count(edit['new'])==1:source=source.replace(edit['new'],edit['old'],1)
  else:assert source.count(edit['old'])==1,('Partial native Hourglass amendment',path)
 return source
@functools.lru_cache(None)
def verify():
 for path,hashes in PLAN['files'].items():
  source=(ROOT/path).read_text(encoding='utf-8-sig')
  pinned=subprocess.check_output(['git','-C',str(ROOT),'show',PLAN['baseline']+':'+path]).decode('utf-8-sig').replace('\r\n','\n')
  assert hashlib.sha256(pinned.encode()).hexdigest()==hashes['before_sha256']
  assert hashlib.sha256(source.encode()).hexdigest()==hashes['after_sha256'],('Undeclared Hourglass amendment',path)
  assert normalize(path,source)==pinned,('Hourglass whole-file restoration differs',path)
 return True
if __name__=='__main__':verify();print('PASS exact native Hourglass amendment; earlier scopes retain independently pinned whole files')
